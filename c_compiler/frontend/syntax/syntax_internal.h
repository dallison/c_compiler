//
//  syntax_internal.h
//  c_compiler
//
//  Declarations shared by the syntax_*.c files that make up the
//  declaration parser.  Not for use outside them.
//
#ifndef syntax_internal_h
#define syntax_internal_h
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include "ast.h"
#include "statement_parser.h"
#include "syntax.h"
#include "type.h"

// syntax.c
bool InNamedNamespace(Syntax* syntax);
Symbol* FindCurrentInheritedInjectedClassName(Syntax* syntax,
                                              String* name);
void MarkCXX26AutomaticNameIndependent(Symbol* symbol);
bool IsDefinition(TypeParser* parser, Symbol* sym, Storage storage);

// syntax_lookahead.c
void ApplyCXXSpecialMemberDeclSpecifiers(TypeParser* parser);
bool CurrentLineLooksLikeSpecialMemberDefinition(Syntax* syntax);

// syntax_overload.c
Symbol* FindFileScopeSymbol(Syntax* syntax, String* name);
void MarkExportedDeclaration(Syntax* syntax, Symbol* symbol);
bool InsertFileScopeSymbol(Syntax* syntax, Symbol* symbol);
bool CanOverloadFunctions(Symbol* a, Symbol* b);
bool SymbolIsHideableClassOrEnumName(Symbol* sym);
void HideClassOrEnumName(Syntax* syntax, Symbol* injected);
void AppendMemberFunctionSpecialization(Syntax* syntax,
                                        StructMember* first,
                                        StructMember* specialization);
void MarkFunctionTemplateSpecialization(Syntax* syntax, Symbol* sym,
                                        Symbol* first_overload);
void MarkMemberFunctionTemplateSpecialization(
    Syntax* syntax, Symbol* sym, StructMember* first_overload);
void DefineExplicitFunctionInstantiation(Syntax* syntax,
                                         TypeParser* parser,
                                         Symbol* sym);
bool RedeclarationTypesEqual(TypeRecord* left, TypeRecord* right);
bool MemberDefinitionTypesEqual(TypeParser* parser, TypeRecord* left,
                                TypeRecord* right);
Symbol* FindMatchingOverload(Symbol* first, TypeRecord* type,
                             Symbol* incoming);
void AppendOverload(Symbol* first, Symbol* overload);
Vector* PendingTemplateParameterList(Syntax* syntax, Symbol* symbol);
ConstraintExpr* PendingFunctionTemplateConstraint(
    Symbol* symbol, Vector* parameters, ConstraintExpr* trailing_constraint);
bool TryAppendSameSignatureConstrainedTemplateOverload(
    Syntax* syntax, Symbol* first, Symbol* overload,
    ConstraintExpr* pending_constraint);
Symbol* FollowAlias(Symbol* symbol);

// syntax_qualified_name.c
Symbol* LookupSymbolInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                       Namespace* ns,
                                                       String* name,
                                                       bool* ambiguous);
Symbol* LookupTagInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                    Namespace* ns,
                                                    String* name,
                                                    bool* ambiguous);
Namespace* FindNamespaceChildInScope(Syntax* syntax, String* name);
bool TemplateArgumentVectorIsDependent(Vector* args);

// syntax_contracts.c
void MergeCXXContractAssertions(Syntax* syntax, Symbol* old_sym,
                                Symbol* new_sym);

// syntax_initializer.c
bool SyntaxExpressionContainsParameterPack(ASTNode* node);
bool CXXTypeContainsParameterPack(Syntax* syntax, TypeRecord* type);
ASTNode* ParseBracedInitializer(Syntax* syntax);
ASTNode* MarkDirectListInitializer(ASTNode* initializer);
const char* CXXConstructorNameForType(TypeRecord* type);
StructMember* FindCXXConstructor(TypeRecord* type);
bool CXXTypeHasDefaultConstructor(TypeRecord* type);
TypeRecord* CXXArrayBaseElementType(TypeRecord* type,
                                    unsigned long* count);
ASTNode* NewCXXArrayElementExpression(Symbol* array,
                                      unsigned long flat_index,
                                      SourceLocation location);
bool CXXConstructorSetHasInitializerList(StructMember* ctor);
bool TypeHasCXXInitializerListConstructor(TypeRecord* type);
ASTNode* NewCXXCopyListConstructorInitializer(Syntax* syntax,
                                                    Symbol* sym,
                                                    ASTNode* initializer);
Vector* ParseCXXInitializerArgumentList(Syntax* syntax, Token close);
bool ResolveCXXClassTemplateArgumentDeductionFromInitializer(
    Syntax* syntax, Symbol* sym, ASTNode* initializer);
ASTNode* ParseCXXDirectInitializer(Syntax* syntax, Symbol* sym,
                                   bool wrap_aggregate_braces);

// syntax_special_member.c
void ResolveOldStyleFormalArgument(Syntax* syntax, TypeRecord* func, Symbol* formal);
void AddFunctionScopeSymbols(Syntax* syntax, TypeRecord* func);
Symbol* FindThisSymbol(Syntax* syntax);  // expr_parser_member.c
bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type);
void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                      bool complete_object,
                                      SourceLocation location);
ASTNode* NewCXXSourceBaseSubobject(Symbol* source,
                                   CXXBaseSpecifier* base,
                                   SourceLocation location);
ASTNode* NewCXXBaseSpecialMemberCall(Syntax* syntax,
                                     TypeRecord* receiver_func,
                                     CXXBaseSpecifier* base,
                                     bool destructor,
                                     bool complete_object,
                                     Vector* actuals,
                                     SourceLocation location);
ASTNode* NewCXXVirtualBaseSpecialMemberCall(Syntax* syntax,
                                            TypeRecord* receiver_func,
                                            CXXVirtualBaseInfo* vbase,
                                            bool destructor,
                                            Vector* actuals,
                                            SourceLocation location);
ASTNode* NewCXXMemberDestructorCall(Syntax* syntax, TypeRecord* func,
                                    StructMember* member,
                                    TypeRecord* object_type,
                                    ASTNode* receiver,
                                    SourceLocation location);
void SyntaxAppendCXXMemberDestructorCalls(Syntax* syntax, TypeRecord* func,
                                           Vector* body,
                                           SourceLocation location);
CXXDeferredConstructorInitializer* NewCXXDeferredConstructorInitializer(
    const char* name, Vector* actuals, SourceLocation location,
    bool is_pack_expansion);
Vector* CloneCXXConstructorInitializerActuals(Vector* actuals);
CXXDeferredConstructorInitializer* CloneCXXDeferredConstructorInitializer(
    CXXDeferredConstructorInitializer* init);
void CXXDeferredConstructorInitializerDelete(
    CXXDeferredConstructorInitializer* init);
void AppendCXXDefaultedMemberwiseAssignments(TypeRecord* func,
                                             Vector* body,
                                             SourceLocation location);
void AppendCXXDefaultedAssignmentReturnThis(TypeRecord* func,
                                            Vector* body,
                                            SourceLocation location);

// syntax_ctor_init.c
void VectorInsertOrAppend(Vector* vec, size_t index, void* value);
bool VectorContainsPointer(Vector* vec, void* value);
Symbol* CXXThisSymbolFromFunction(TypeRecord* func);
ASTNode* NewCXXThisMemberAccess(TypeRecord* func,
                                const char* member_name,
                                SourceLocation location);
Symbol* CXXSourceObjectParameter(TypeRecord* func);
ASTNode* NewCXXSourceMemberAccess(Symbol* source,
                                  const char* member_name,
                                  SourceLocation location);
ASTNode* NewCXXDefaultMemberInitializerStatement(Syntax* syntax,
                                                       TypeRecord* func,
                                                       StructMember* member);
ASTNode* FindCXXExplicitMemberInitializer(CXXConstructorInitList* init_list,
                                                StructMember* member);
bool InsertCXXAnonymousAggregateInitializers(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list,
    StructMember* anon, Vector* body, size_t* insert_at);
ASTNode* FindCXXExplicitVirtualBaseInitializer(
    CXXConstructorInitList* init_list, CXXVirtualBaseInfo* base);

// syntax_vptr.c
// A constructor's __vptr initialization that could not be emitted when its
// preamble was built (because the class's vtables had not been registered yet),
// to be inserted into `body` at `index` once SyntaxFlushPendingVPtrInitializers
// runs for `owner`.
typedef struct {
  Struct* owner;
  TypeRecord* func;
  size_t index;
  SourceLocation location;
} PendingVPtrInit;
extern Vector pending_vptr_inits;
extern bool pending_vptr_inits_initialized;
void AppendCXXVPtrInitializers(TypeRecord* func, Vector* body,
                               SourceLocation location);
void AppendCXXVBPtrInitializers(TypeRecord* func, Vector* body,
                                SourceLocation location);
ASTNode* NewCXXCompleteObjectGuardedStatement(TypeRecord* func,
                                                    Vector* statements,
                                                    SourceLocation location);
void InsertCXXCompleteObjectGuardedStatements(TypeRecord* func,
                                                    Vector* body,
                                                    size_t* insert_at,
                                                    Vector* statements,
                                                    SourceLocation location);

// syntax_ctor_preamble.c
// An array with more base elements than this is constructed or destroyed by a
// loop instead of one call per element, which would make the expression as
// deep as the array is long.
enum { kCXXUnrolledArrayElementLimit = 16 };
ASTNode* NewCXXElementSpecialMemberCall(ASTNode* element,
                                        TypeRecord* element_type,
                                        const char* name,
                                        SourceLocation location);
ASTNode* NewCXXArrayElementLoop(Syntax* syntax, ASTNode* first,
                                TypeRecord* element_type,
                                unsigned long count, const char* name,
                                bool reverse, SourceLocation location);
ASTNode* NewCXXMemberInitializerStatement(Syntax* syntax,
                                                TypeRecord* func,
                                                StructMember* member,
                                                Vector* actuals,
                                                bool value_initialize_empty,
                                                SourceLocation location);

// syntax_function_decl.c
ASTNode* CloneInitializer(ASTNode* init);
void MergeCXXDefaultArguments(Syntax* syntax, Symbol* old_sym,
                              Symbol* new_sym);
void ParseCXXDefaultDeleteFunctionSpecifier(Syntax* syntax,
                                            TypeRecord* func,
                                            bool is_first_declaration);
ASTNode* DeclareOrDefineFunction(Syntax* syntax,
                                 Vector* declarations,
                             Symbol* sym, Symbol* old_sym);
void SkipFunctionBody(Syntax* syntax);

// syntax_decl_specifier.c
bool IsPowerOf2OrZero(int32_t v);
void DiagnoseConstevalOnNonFunction(Syntax* syntax, TypeRecord* type,
                                    bool is_consteval);
void CheckMatchingConstexprConsteval(Syntax* syntax,
                                     TypeRecord* previous,
                                     TypeRecord* current);
void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage,
                               bool* is_inline, bool* is_constexpr,
                               bool* is_consteval, bool* is_constinit,
                               bool* is_explicit, TypeRecord** type,
                               Vector* attributes,
                               ParserContext context);
Symbol* CXXClassTemplateOrigin(TypeRecord* type);
bool TryParseCXXDeductionGuide(Syntax* syntax, Symbol* sym);

// syntax_declaration_checks.c
bool IsC23InferredAutoType(TypeRecord* type);
void ValidateC23AutoDeclarator(Syntax* syntax, Symbol* sym);
void ValidateC23AutoInitializer(Syntax* syntax, Symbol* sym,
                                ASTNode* initializer);
void ValidateC23ConstexprObject(Syntax* syntax, Symbol* sym,
                                ASTNode* initializer);
void ApplyExplicitCXXLinkageAttachment(Syntax* syntax, Symbol* sym,
                                       Storage storage);
void ValidateCXX26MainDeclaration(Syntax* syntax, Symbol* sym,
                                  Storage storage);
void ValidateCXX26DeletedMain(Syntax* syntax, Symbol* sym);
bool CXXConstVariableHasInternalLinkage(Syntax* syntax,
                                        TypeParser* parser, Symbol* sym,
                                        Symbol* old_sym);
bool CXXDeclarationInUnnamedNamespace(Syntax* syntax,
                                      TypeParser* parser, Symbol* sym,
                                      Storage storage);
bool AsmLabelIsMachOSymbolName(Syntax* syntax);

// syntax_external_decl.c
void AppendDeclarationsFromNode(Vector* declarations, ASTNode* node);
ASTNode* EmptyDeclarationList(SourceLocation location);
void CheckDeclaredObjectType(Syntax* syntax, Symbol* sym);

// syntax_namespace.c
bool LookingAtNamespaceAliasDefinition(Syntax* syntax);
ASTNode* ParseNamespaceAliasDefinition(Syntax* syntax,
                                       SourceLocation location);
ASTNode* ParseUsingDeclaration(Syntax* syntax);
ASTNode* ParseNamespaceDeclaration(Syntax* syntax, bool leading_inline);

// syntax_template_param.c
TemplateParameter* NewTemplateParameter(const char* name,
                                        TemplateParameterKind kind,
                                        bool is_parameter_pack,
                                        TypeRecord* type,
                                        TypeRecord* default_type,
                                        bool has_default_int,
                                        long long default_int_value,
                                        int default_template_parameter_index,
                                        int index);
bool ExpressionContainsDependentTemplateParameter(ASTNode* node);
bool ExpressionIsNonTypeTemplateParameter(ASTNode* node,
                                          int* parameter_index);

// syntax_template_arg.c
bool SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(Syntax* syntax);
bool SymbolNamesTemplateArgument(Symbol* symbol);

// syntax_template_decl.c
bool TypeIsClassTemplateObject(TypeRecord* type);
const char* ClassTemplateObjectName(TypeRecord* type);
void MaterializeDeferredClassTemplateType(Syntax* syntax, Symbol* sym);
int CurrentTemplateParameterListLength(Syntax* syntax);
int CurrentTemplateParameterBase(Syntax* syntax);
void MoveCurrentTemplateParametersToFunction(Syntax* syntax,
                                             TypeRecord* func);
void AddFunctionAssociatedConstraint(TypeRecord* func,
                                     ConstraintExpr* constraint);
void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                  ConstraintExpr* constraint);
void MoveTemplateParameterConstraints(Vector* parameters,
                                      ConstraintExpr** target);
ASTNode* ParseExplicitTemplateInstantiation(Syntax* syntax,
                                            SourceLocation location,
                                            bool is_extern);
ASTNode* ParseTemplateDeclaration(Syntax* syntax);

// syntax_object_lifetime.c
void SetCXXInlineLocalStaticAsmName(Symbol* sym, const char* suffix);
bool CXXThreadLocalInitializerIsDynamic(ASTNode* initializer);
ASTNode* NewCXXThreadLocalGuardedInit(Syntax* syntax, Symbol* sym,
                                      ASTNode* init);
ASTNode* NewCXXThreadLocalGuardedConstructor(Syntax* syntax,
                                             Symbol* sym);

// syntax_local_decl.c
ASTNode* NewVariableInitExpression(Syntax* syntax, Symbol* sym,
                                   ASTNode* initializer);
bool CXXDeferConstexprDefaultInitialization(Syntax* syntax,
                                            TypeRecord* type);

// Defined outside the syntax_*.c files.
void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg, int base);
void RebaseTemplateParameterIndices(TypeRecord* type, int base);

#endif /* syntax_internal_h */
