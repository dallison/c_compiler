//
//  syntax.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef syntax_h
#define syntax_h

#include <stdarg.h>
#include <setjmp.h>

#include "ast.h"
#include "dstring.h"
#include "lex.h"
#include "symbol_table.h"
#include "vector.h"
#include "parser_context.h"

struct ConstraintExpr;
struct Struct;

extern jmp_buf error_abort_state;       // Where to abort to.
extern bool abort_on_error;

// The syntax analyzer.
typedef struct Syntax {
  Lex* lex;                             // Lexical analyzer providing tokens.
  ASTNode* ast;                         // Output AST.
  LocalSymbolTable* local_symbol_stack; // Local symbol tables.
  LocalSymbolTable* local_tag_stack;    // Local tags.
  Namespace* current_namespace;         // Current C++ namespace scope.
  char fake_name_buffer[32];            // Buffer to generate fake names.
  int fake_name_index;                  // Next fake name index.
  bool found_open_paren;                // We've consumed an open paren.
  TypeRecord* compound_literal_type;            // Parsed compound literal type.
  int loop_count;                       // Number of nested loops.
  int switch_count;                     // Number of nested switch statements.
  Vector all_local_symbols;  // All symbols defined in a function (owned by this
                             // vector).
  Vector local_statics;      // All local statics defined in function.
  Vector inline_static_member_definitions;  // Inline static data member defs.
  Vector all_symbols;        // Local symbols of all prior declarations, moved
                             // here by SyntaxResetForNewDeclaration so they
                             // outlive the reset; owned by this vector and freed
                             // at SyntaxDestruct.
  Symbol* last_parsed_tag;   // Most recent struct/union tag parsed as a type.
  bool parsing_template_declaration;  // Parsing declaration after template<...>.
  bool parsing_template_specialization;  // Parsing declaration after template<>.
  bool parsing_template_argument;  // Parsing expression inside template args.
  int current_template_parameter_count;  // Type params for current template.
  Vector* current_template_parameters;  // TemplateParameter* for current template.
  struct ConstraintExpr* current_template_requires_clause;  // C++20 requires.
  struct ASTNode* pending_explicit_condition;  // Deferred value-dependent explicit(bool).
  struct ConstraintExpr* pending_placeholder_variable_constraint;  // `Concept auto x`.
  struct Struct* cxx_class_head;  // Class whose base-clause/body is being parsed;
                                  // lets self-template-ids resolve inside nested
                                  // template-argument parsing that spins up fresh
                                  // TypeParsers lacking cxx_member_owner.
  
  ParserContext context;     // Parser context.
  Storage init_storage;      // Current storage for symbol being initialized.
  int extern_c_depth;        // Nesting depth of `extern "C"` linkage specs.
  int export_depth;          // Nesting depth of C++20 `export` regions.
} Syntax;

typedef struct FullyQualifiedIdentifier {
  bool absolute;       // Starts with ::.
  bool is_qualified;   // Contains :: or starts with ::.
  Vector components;   // String* components, owned by this object.
  Vector template_arguments;  // Vector* template args per component, owned.
  String spelling;     // Full spelling for diagnostics.
} FullyQualifiedIdentifier;

typedef struct CXXDeferredConstructorInitializer {
  String name;
  Vector* actuals;
  SourceLocation location;
} CXXDeferredConstructorInitializer;

typedef struct CXXConstructorInitList {
  Vector virtual_base_specs;  // CXXVirtualBaseInfo*; not owned.
  Vector virtual_base_statements;  // ASTNode*; transferred into function body.
  Vector base_specs;         // CXXBaseSpecifier*; not owned.
  Vector base_statements;    // ASTNode*; transferred into function body.
  Vector member_specs;       // StructMember*; not owned.
  Vector member_statements;  // ASTNode*; transferred into function body.
  Vector raw_initializers;  // CXXDeferredConstructorInitializer*; owned.
  Vector deferred_initializers;  // CXXDeferredConstructorInitializer*; owned.
  int last_initializer_order;
} CXXConstructorInitList;

// Token classes allow us to recover from syntax errors by
// skipping tokens until the current token matches a certain
// class.
#define TC(x) kTokenClass##x
typedef enum TokenClass {
  TC(stmt) = 1 << 0,            // A statement.
  TC(type) = 1 << 1,            // A type.
  TC(expr) = 1 << 2,            // An expression.
  TC(closebra) = 1 << 3,        // A closing bracket.
  TC(closebrace) = 1 << 4,      // A closing brace.
  TC(openbra) = 1 << 5,         // An opening bracket.
  TC(exprsep) = 1 << 6,         // An expression separator.
  TC(decl) = 1 << 7,            // A declaration.
  TC(semicolon) = 1 << 8,       // A semicolon
} TokenClass;

// Given a token, what class does it belong to?
TokenClass ClassifyToken(Token tok);

// True when an analyzed expression is value-dependent on a template parameter
// (e.g. `sizeof(T) > 4`), i.e. it cannot be constant-folded until instantiation.
bool ExpressionIsTemplateDependent(struct ASTNode* expr);

void SyntaxInit(Syntax* syntax, Lex* lex);
void SyntaxDestruct(Syntax* syntax);
void SyntaxResetForNewDeclaration(Syntax* syntax);

bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol);
// Adds a symbol to the current lookup scope without taking ownership. The
// caller must keep the symbol alive until the scope closes.
bool SyntaxAddBorrowedSymbol(Syntax* syntax, Symbol* symbol);
Symbol* SyntaxFindSymbol(Syntax* syntax, String* name);
Symbol* SyntaxFindTopScopeSymbol(Syntax* syntax, String* name);
void FullyQualifiedIdentifierInit(FullyQualifiedIdentifier* name);
void FullyQualifiedIdentifierDestruct(FullyQualifiedIdentifier* name);
bool SyntaxParseFullyQualifiedIdentifier(Syntax* syntax,
                                         FullyQualifiedIdentifier* name);
bool SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
    Syntax* syntax, FullyQualifiedIdentifier* name, TokenClass followers);
bool SyntaxParseOperatorFunctionName(Syntax* syntax, String* name);
bool SyntaxIsCXXNumericLiteralOperatorTemplate(Symbol* symbol);
// Parses an operator-function-id (operator+, operator(), operator[], ...) or a
// conversion-function-id (operator <type-id>) used as the member name in an
// explicit member access such as `x.operator+` or `p->operator int`.  The lexer
// must be positioned at the `operator` keyword.  On success `name` (which the
// function initializes) holds the registered member name, matching the names
// produced at declaration time.  Returns false (leaving `name` untouched) if not
// positioned at `operator`.
bool SyntaxParseMemberOperatorName(Syntax* syntax, String* name);
ASTNode* SyntaxParseStaticAssert(Syntax* syntax);
// Parses a C++ 'friend' declaration appearing inside the body of class
// 'befriending'.  Handles friend class declarations ('friend class X;' and
// 'friend X;') as well as friend function declarations and inline friend
// function definitions, recording the granted friendships on 'befriending'.
void SyntaxParseFriendDeclaration(Syntax* syntax, Struct* befriending);
// Registers a fully-substituted instantiated friend function in namespace 'ns'
// (merging overloads), returning the symbol that persists in scope.  Used when
// instantiating a class template's deferred friend functions.
Symbol* SyntaxRegisterInstantiatedFriendFunction(Syntax* syntax, Namespace* ns,
                                                 Symbol* sym);
Vector* SyntaxParseTemplateArgumentList(Syntax* syntax, TokenClass followers);
Vector* SyntaxParseTemplateParameterList(Syntax* syntax);
Vector* SyntaxParseTemplateParameterListWithBase(Syntax* syntax, int base);
Symbol* SyntaxFindQualifiedSymbol(Syntax* syntax,
                                  FullyQualifiedIdentifier* name);
Symbol* SyntaxFindQualifiedPrefixSymbol(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        size_t component_count);
Symbol* SyntaxFindQualifiedTag(Syntax* syntax,
                               FullyQualifiedIdentifier* name);
Namespace* SyntaxFindQualifiedNamespace(Syntax* syntax,
                                        FullyQualifiedIdentifier* name);
const char* FullyQualifiedIdentifierLast(FullyQualifiedIdentifier* name);
bool SyntaxCurrentTokenStartsQualifiedName(Syntax* syntax);
bool SyntaxCurrentIdentifierFollowedByScopeOperator(Syntax* syntax);
bool SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(Syntax* syntax);

bool SyntaxAddTag(Syntax* syntax, Symbol* symbol);
Symbol* SyntaxFindTag(Syntax* syntax, String* name);
Symbol* SyntaxFindTopScopeTag(Syntax* syntax, String* name);

void SyntaxOpenScope(Syntax* syntax);
void SyntaxCloseScope(Syntax* syntax);

// Make the non-static data members (and member typedefs) of `owner` visible as
// unqualified names in the current (already-open) scope.  Used so a member
// function's trailing requires-clause can name earlier-declared members (which
// are accessible there via the implicit object parameter), e.g.
// `bool empty() const requires requires { ranges::empty(*__r_); }`.
void SyntaxInsertClassMembersForConstraint(Syntax* syntax, struct Struct* owner);

const char* SyntaxFakeName(Syntax* syntax);
void SyntaxFakeTagName(Syntax* syntax, String* tag_name);

void SyntaxError(Syntax* syntax, const char* format, ...);
void SyntaxErrorAtLocation(Syntax* syntax, SourceLocation location,
                           const char* format, ...);
void SyntaxWarning(Syntax* syntax, const char* warn, const char* format, ...);
Storage SyntaxParseStorage(Syntax* syntax);

ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax);
ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax);
ASTNode* SyntaxParseConditionDeclaration(Syntax* syntax);
void SyntaxPrepareCXXLocalStatics(Syntax* syntax, TypeRecord* function);
ASTNode* SyntaxNewCXXGlobalAtexitStatement(Symbol* sym, SourceLocation location);
void SyntaxRegisterFunctionInitFiniAttributes(Syntax* syntax, Symbol* sym);

void SyntaxNeedSemicolon(Syntax* syntax, TokenClass followers);
void SyntaxNeedBracket(Syntax* syntax, Token bracket, TokenClass followers);

// Closes a template-argument or template-parameter list, splitting a merged
// `>>` / `>>=` / `>=` token so nested template-ids like `vector<vector<int>>`
// parse correctly (C++11 CWG N1757).  Emits a diagnostic and recovers if the
// current token does not close the list.
void SyntaxNeedTemplateClose(Syntax* syntax, TokenClass followers);
void SyntaxRecover(Syntax* syntax, TokenClass tc);
void SyntaxCheckThreadLocal(Syntax* syntax, Symbol* symbol,
                            ParserContext context, bool is_static_member,
                            bool is_nonstatic_member);
bool SyntaxLookingAtType(Syntax* syntax);
bool SyntaxCurrentClassNameStartsType(Syntax* syntax);
bool SyntaxLookingAtDeclaration(Syntax* syntax);

Symbol* SyntaxNewTemporary(Syntax* syntax, struct TypeRecord* type);
ASTNode* SyntaxNewPCLabel(SourceLocation location);
ASTNode* SyntaxNewCXXDefaultConstructorCallIfNeeded(Syntax* syntax,
                                                    Symbol* sym);
ASTNode* SyntaxRewriteCXXCopyInitConstructorIfNeeded(Syntax* syntax,
                                                     Symbol* sym,
                                                     ASTNode* initializer);
ASTNode* SyntaxParseBracedInitializer(Syntax* syntax);
ASTNode* SyntaxParseInitializer(Syntax* syntax, Symbol* sym, Storage storage);
ASTNode* SyntaxParseCXXDefaultMemberInitializer(Syntax* syntax);
void SyntaxParseAttribute(Syntax* syntax, Vector* attrs);
bool SyntaxLookingAtCXXAttribute(Syntax* syntax);
bool SyntaxParseCXXAttributes(Syntax* syntax, Vector* attrs);
bool SyntaxParseCXXAlignas(Syntax* syntax, Vector* attrs);
void SyntaxApplyDeclarationAttributes(Symbol* sym);
void SyntaxCXXConstructorInitListInit(CXXConstructorInitList* init_list);
void SyntaxCXXConstructorInitListDestruct(CXXConstructorInitList* init_list);
CXXConstructorInitList* SyntaxCXXConstructorInitListCloneRaw(
    CXXConstructorInitList* init_list);
CXXConstructorInitList* SyntaxCXXConstructorInitListCloneDeferred(
    CXXConstructorInitList* init_list);
void SyntaxCXXConstructorInitListRemapFormals(CXXConstructorInitList* init_list,
                                              TypeRecord* from_func,
                                              TypeRecord* to_func,
                                              int rebase_base);
void SyntaxParseCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list);
void SyntaxResolveCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list);
void SyntaxInsertCXXConstructorPreamble(
    Syntax* syntax, TypeRecord* func, Vector* body,
    CXXConstructorInitList* init_list, SourceLocation location);
ASTNode* SyntaxNewCXXMemberInitializerStatement(
    Syntax* syntax, TypeRecord* func, StructMember* member, Vector* actuals,
    SourceLocation location);
// Appends the base-class subobject destructor calls that must run at the end of
// a destructor body (after the class's own members are destroyed).  Shared with
// the inline / implicitly-defined destructor paths in the type subsystem so
// those bodies destroy their base subobjects too.
void AppendCXXBaseDestructorCalls(Syntax* syntax, TypeRecord* func,
                                  Vector* body, SourceLocation location);
// Appends the base-subobject copy/move assignments for a defaulted copy/move
// assignment operator, forwarding to each direct base's operator=.  The
// memberwise-copy helper only assigns the class's own members, so a derived
// class needs this to assign its inherited base subobjects too.
void AppendCXXBaseAssignments(Syntax* syntax, TypeRecord* func, Vector* body,
                              SourceLocation location);
// Emits the __vptr initializers that were deferred (see the preamble) for
// constructors of `owner`, now that its vtables have been registered.
void SyntaxFlushPendingVPtrInitializers(struct Struct* owner);

#endif /* syntax_h */
