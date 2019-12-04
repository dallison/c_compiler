//
//  ast.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

// Abstract Syntax Tree (AST) declarations.

#ifndef ast_h
#define ast_h

#include "symbol.h"
#include "tokens.h"
#include "type.h"

#define AST_OP(op) kASTOpcode_##op
typedef enum {
  AST_OP(bad),
  AST_OP(number),
  AST_OP(identifier),
  AST_OP(string),
  AST_OP(string_wide),
  AST_OP(charconst),
  AST_OP(fnumber),
  AST_OP(postinc),
  AST_OP(postdec),
  AST_OP(uminus),
  AST_OP(uplus),
  AST_OP(contents),
  AST_OP(address),
  AST_OP(rshiftl),
  AST_OP(rshifta),
  AST_OP(rshifteql),
  AST_OP(rshifteqa),
  AST_OP(and),
  AST_OP(andeq),
  AST_OP(arrow),
  AST_OP(assign),
  AST_OP(not),
  AST_OP(bitor),
  AST_OP(break),
  AST_OP(exor),
  AST_OP(exoreq),
  AST_OP(case),
  AST_OP(charwide),
  AST_OP(colon),
  AST_OP(comma),
  AST_OP(complex),
  AST_OP(continue),
  AST_OP(do),
  AST_OP(dot),
  AST_OP(equal),
  AST_OP(for),
  AST_OP(goto),
  AST_OP(greater),
  AST_OP(greatereq),
  AST_OP(if),
  AST_OP(imaginary),
  AST_OP(compound),
  AST_OP(less),
  AST_OP(lesseq),
  AST_OP(logand),
  AST_OP(logor),
  AST_OP(call),
  AST_OP(lshift),
  AST_OP(lshifteq),
  AST_OP(subscript),
  AST_OP(minus),
  AST_OP(minuseq),
  AST_OP(predec),
  AST_OP(noteq),
  AST_OP(oreq),
  AST_OP(mod),
  AST_OP(percenteq),
  AST_OP(plus),
  AST_OP(pluseq),
  AST_OP(preinc),
  AST_OP(question),
  AST_OP(return),
  AST_OP(rshift),
  AST_OP(rshifteq),
  AST_OP(sizeof),
  AST_OP(div),
  AST_OP(diveq),
  AST_OP(mult),
  AST_OP(multeq),
  AST_OP(switch),
  AST_OP(onescomp),
  AST_OP(while),
  
  AST_OP(asm),
  AST_OP(attribute),
  
  // stdarg builtins.
  AST_OP(builtin_va_start),
  AST_OP(builtin_va_arg),
  AST_OP(builtin_va_end),
  AST_OP(builtin_va_copy),

  AST_OP(cast),   // Cast AST node token.
  AST_OP(label),  // Label AST node.
  AST_OP(vardecl),  // Variable declaration.
  AST_OP(decl_list),  // Declaration list
  AST_OP(macro),      // Macro in processor mode expressions.
  AST_OP(expr),       // Expression statement.
  AST_OP(init),       // Initialization.
  AST_OP(expr_init),
  AST_OP(braced_init),
  AST_OP(designated_init),
  
  // Type conversions.
  // Integer to...
  AST_OP(i2s),
  AST_OP(i2c),
  AST_OP(i2l),
  AST_OP(i2ll),
  AST_OP(i2f),
  AST_OP(i2d),
  AST_OP(i2ld),
  AST_OP(i2b),
  
  // Character to...
  AST_OP(c2i),
  AST_OP(c2s),
  AST_OP(c2l),
  AST_OP(c2ll),
  AST_OP(c2f),
  AST_OP(c2d),
  AST_OP(c2ld),
  AST_OP(c2b),
  
  // Short to...
  AST_OP(s2i),
  AST_OP(s2c),
  AST_OP(s2l),
  AST_OP(s2ll),
  AST_OP(s2f),
  AST_OP(s2d),
  AST_OP(s2ld),
  AST_OP(s2b),
  
  // Long to...
  AST_OP(l2i),
  AST_OP(l2c),
  AST_OP(l2s),
  AST_OP(l2ll),
  AST_OP(l2f),
  AST_OP(l2d),
  AST_OP(l2ld),
  AST_OP(l2b),
  
  // Long long to...
  AST_OP(ll2i),
  AST_OP(ll2c),
  AST_OP(ll2s),
  AST_OP(ll2l),
  AST_OP(ll2f),
  AST_OP(ll2d),
  AST_OP(ll2ld),
  AST_OP(ll2b),
  
  // Float to...
  AST_OP(f2i),
  AST_OP(f2c),
  AST_OP(f2s),
  AST_OP(f2l),
  AST_OP(f2ll),
  AST_OP(f2d),
  AST_OP(f2ld),
  AST_OP(f2b),
  
  // Double to...
  AST_OP(d2i),
  AST_OP(d2c),
  AST_OP(d2s),
  AST_OP(d2l),
  AST_OP(d2ll),
  AST_OP(d2f),
  AST_OP(d2ld),
  AST_OP(d2b),
  
  // Long double to...
  AST_OP(ld2i),
  AST_OP(ld2c),
  AST_OP(ld2s),
  AST_OP(ld2l),
  AST_OP(ld2ll),
  AST_OP(ld2f),
  AST_OP(ld2d),
  AST_OP(ld2b),
  
  // Bool to...
  AST_OP(b2i),
  AST_OP(b2c),
  AST_OP(b2s),
  AST_OP(b2l),
  AST_OP(b2ll),
  AST_OP(b2f),
  AST_OP(b2d),
  AST_OP(b2ld),
} ASTOpcode;

const char* ASTOpcodeName(ASTOpcode op);

struct ASTNode;

typedef void (*ASTNodeDeleter)(struct ASTNode* node);
typedef void (*ASTNodePrinter)(struct ASTNode* node, int indents);
typedef void (*ASTNodeChildReplacer)(struct ASTNode* parent, int child_id,
                                     struct ASTNode* child,
                                     bool delete_old_child);

// This is a table of pointers to functions that are provided at runtime
// to implement late-bound functions (a.k.a virtual functions) for an
// AST node.
typedef struct {
  ASTNodeDeleter deleter;  // Function to delete the node.
  ASTNodePrinter printer;  // Function to print the node.
  ASTNodeChildReplacer replacer;
} ASTNodeVirtuals;

// Abstract Syntax Tree (AST) node.
// This is an object-oriented struct that meant to be embedded as a
// base in other structs that extend the type.  In C++ this would
// be a base class.  It contains the common members for an AST node
// and a pointer to a table of function pointers.  This function pointers
// are the equivalent of a C++ virtual function in that it is set at runtime
// base on the type of the node being created.
//
// Each AST node has an identifiying field called 'op'.

typedef struct ASTNode {
  ASTOpcode op;               // Opcode.
  int flags;                  // Flags
  TypeRecord* type;           // Node type (mostly set by semantic analyzer)
  struct ASTNode* parent;     // Parent node (if any).
  int child_id;               // Which child am I in the parent node?
  SourceLocation location;    // Location in input.
  ASTNodeVirtuals* virtuals;  // Virtual table (statically allocated, do not free).
} ASTNode;

// Flags for ASTNode.
#define kASTNeedAddress 1     // Need address, not value.
#define kASTIsDeclaration 2   // Identifier is a declaration, not reference.
#define kASTStaticInit 4      // Initialization is for a static variable.
#define kASTStatementStart 8  // Start of a statement.

// Initialize an AST node.
void ASTNodeInit(ASTNode* node, ASTOpcode op, TypeRecord* type,
                 SourceLocation location, ASTNodeVirtuals* virtuals);

// Creates a new AST node.  The deleter for this will be ASTNodeBaseDelete
// which simply frees the memory passed.
ASTNode* NewASTNode(ASTOpcode op, TypeRecord* type, SourceLocation location);

// Deletes an AST node based on its runtime type.  It calls the deleter
// function pointer in the node.
void ASTNodeDelete(ASTNode* node);
void ASTNodeSetType(ASTNode* node, TypeRecord* type);
void ASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                         bool delete_old_child);
void ASTNodePrint(ASTNode* node, int indents);

bool ASTNodeIsIntConstant(ASTNode* node);
int64_t ASTNodeConstantValue(ASTNode* node);

// A unary AST node with a single child.
typedef struct {
  ASTNode base;
  struct ASTNode* sub;
} UnaryASTNode;

ASTNode* NewUnaryASTNode(ASTOpcode op, TypeRecord* type,
                         SourceLocation location, ASTNode* sub);

// A binary AST node with left and right children.
typedef struct {
  ASTNode base;
  struct ASTNode* left;
  struct ASTNode* right;
} BinaryASTNode;

ASTNode* NewBinaryASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          ASTNode* right);

// An AST node with a left node and vector of children.
typedef struct {
  ASTNode base;
  ASTNode* left;
  Vector* children;
} VectorASTNode;

ASTNode* NewVectorASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          Vector* children);

// An identifier node containing a symbol pointer.  The symbol is not deleted
// when the node is deleted.
typedef struct {
  ASTNode base;
  Symbol* symbol;
} IdentifierASTNode;

ASTNode* NewIdentifierASTNode(Symbol* symbol, SourceLocation location);

typedef struct {
  ASTNode base;
  StructMember* member;
} StructMemberASTNode;

ASTNode* NewStructMemberASTNode(StructMember* member, SourceLocation location);
bool IsBitfieldReference(ASTNode* node);

// A constant node.  This contains a constant that can be either an integer,
// floating point number or a string.  The type in the base discriminates
// the union (says which member is valid).
typedef struct {
  ASTNode base;
  union {
    int64_t ivalue;
    double fvalue;
    String* string;
  } value;
} ConstantASTNode;

ASTNode* NewIntConstantASTNode(int64_t value, TypeRecord* type,
                               SourceLocation location);
ASTNode* NewRealConstantASTNode(double value, TypeRecord* type,
                                SourceLocation location);
ASTNode* NewStringConstantASTNode(String* value, TypeRecord* type,
                                  SourceLocation location);
ASTNode* NewCharConstantASTNode(int value, TypeRecord* type,
                                SourceLocation location);

void IntConstantASTNodeInit(ConstantASTNode* node, int64_t value,
                            TypeRecord* type, SourceLocation location);

// AST node holding a type cast of an expression to a particular type.
typedef struct {
  ASTNode base;
  TypeRecord* cast_type;
  ASTNode* expr;
} CastASTNode;

ASTNode* NewCastASTNode(TypeRecord* type, SourceLocation location,
                        ASTNode* expr);

// Sizeof operation.
typedef struct {
  ConstantASTNode base;
  ASTNode* expr;
} SizeofASTNode;

ASTNode* NewSizeofASTNodeWithKnownSize(int size, SourceLocation location);
ASTNode* NewSizeofASTNodeWithExpression(ASTNode* expr, SourceLocation location);

// Macro name.
typedef struct {
  ASTNode base;
  String macro_name;
} MacroNameASTNode;

ASTNode* NewMacroNameASTNode(String* macro_name, SourceLocation location);

// Expression statement.
typedef struct {
  ASTNode base;
  ASTNode* expr;
} ExpressionStatementASTNode;

ASTNode* NewExpressionStatementASTNode(ASTNode* expr, SourceLocation location);

// If statement with condition, if and else parts.  The else part is optional.
typedef struct {
  ASTNode base;
  ASTNode* cond;
  ASTNode* if_part;
  ASTNode* else_part;
} IfStatementASTNode;

ASTNode* NewIfStatementASTNode(ASTNode* cond, ASTNode* if_part,
                               ASTNode* else_part, SourceLocation location);

// While and do statements.
// These all contain a condition and a statement.
typedef struct {
  ASTNode base;
  ASTNode* cond;
  ASTNode* stmt;
} CombinedStatementASTNode;

ASTNode* NewCombinedStatementASTNode(ASTOpcode tok, ASTNode* cond,
                                     ASTNode* stmt, SourceLocation location);

// Compound statment, containing a vector of statements.
typedef struct {
  ASTNode base;
  Vector* statements;
} CompoundStatementASTNode;

ASTNode* NewCompoundStatementASTNode(Vector* statements,
                                     SourceLocation location);

// For statement.  All expressions (e1, e2 and e3) are optional.
typedef struct {
  ASTNode base;
  ASTNode* c1;
  ASTNode* c2;
  ASTNode* c3;
  ASTNode* stmt;
} ForStatementASTNode;

ASTNode* NewForStatementASTNode(ASTNode* c1, ASTNode* c2, ASTNode* c3,
                                ASTNode* stmt, SourceLocation location);

// Variable declaration, optionally initializing the symbol.
typedef struct {
  ASTNode base;
  Symbol* symbol;
  ASTNode* initializer;  // Assignment expression to initialize variable.
} VariableDeclarationASTNode;

ASTNode* NewVariableDeclarationASTNode(Symbol* symbol, ASTNode* initializer,
                                       SourceLocation location);

// Declaration list, containing a vector of variable declarations.
typedef struct {
  ASTNode base;
  Vector* declarations;
} DeclarationListASTNode;

ASTNode* NewDeclarationListASTNode(Vector* declarations,
                                   SourceLocation location);

// A case label.
// If the 'expr' is NULL this is used as a default label.
typedef struct {
  ASTNode base;
  ASTNode* expr;
  int64_t value;
  struct IRNode* label;
} CaseLabelASTNode;

ASTNode* NewCaseLabelASTNode(ASTNode* initializer, SourceLocation location);

// Switch statement.
typedef struct {
  ASTNode base;
  ASTNode* expr;
  ASTNode* stmt;
  Vector cases;
  CaseLabelASTNode* default_node;
  float density;
  int64_t min_case_value;
  int64_t max_case_value;
} SwitchStatementASTNode;

ASTNode* NewSwitchStatementASTNode(ASTNode* expr, ASTNode* stmt,
                                   SourceLocation location);

// Label.
typedef struct {
  ASTNode base;
  String name;
  struct IRNode* label;
} LabelASTNode;

ASTNode* NewLabelASTNode(const char* name, SourceLocation location);

typedef struct {
  ASTNode base;
  String* text;
  bool is_volatile;
} AsmASTNode;

ASTNode* NewAsmASTNode(String* text, bool is_volatile, SourceLocation location);

//
// Initialization
//

// Initialization of a single expression.
typedef struct {
  ASTNode base;
  ASTNode* expr;
} ExpressionInitializerASTNode;

ASTNode* NewExpressionInitializerASTNode(ASTNode* expr,
                                         SourceLocation location);

// Initialization using a braced enclosed list of expressions.
typedef struct {
  ASTNode base;
  Vector* initializers;
} BracedInitializerASTNode;

ASTNode* NewBracedInitializerASTNode(Vector* initializers,
                                     SourceLocation location);

// Designated initializer, specifying a struct member or array index and an
// initailizer for it.
typedef enum {
  kDesignatorArray,
  kDesignatorStruct,
} DesignatorType;

typedef struct {
  DesignatorType designator_type;
  TypeRecord* type;
  union {
    int array_index;
    String* struct_member_name;   // Before semantic analysis.
    StructMember* struct_member;  // After semantic analysis.
  } value;
} Designator;

Designator* NewArrayDesignator(TypeRecord* type, int index);
Designator* NewStructDesignator(String* member);
Designator* NewStructMemberDesignator(StructMember* member);

typedef struct {
  ASTNode base;
  Vector* designators;
  ASTNode* init;
} DesignatedInitializerASTNode;

ASTNode* NewDesignatedInitializerASTNode(Vector* designators, ASTNode* init,
                                         SourceLocation location);

#endif /* ast_h */
