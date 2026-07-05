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
  AST_OP(try),
  AST_OP(catch),
  AST_OP(less),
  AST_OP(lesseq),
  AST_OP(spaceship),
  AST_OP(logand),
  AST_OP(logor),
  AST_OP(call),
  AST_OP(inline_call),
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
  AST_OP(co_return),
  AST_OP(throw),
  AST_OP(co_await),
  AST_OP(co_yield),
  AST_OP(rshift),
  AST_OP(rshifteq),
  AST_OP(sizeof),
  AST_OP(typeid),
  AST_OP(div),
  AST_OP(diveq),
  AST_OP(mult),
  AST_OP(multeq),
  AST_OP(switch),
  AST_OP(onescomp),
  AST_OP(while),
  AST_OP(structmember),
  
  AST_OP(asm),
  AST_OP(attribute),
  
  // stdarg builtins.
  AST_OP(builtin_va_start),
  AST_OP(builtin_va_arg),
  AST_OP(builtin_va_end),
  AST_OP(builtin_va_copy),
  AST_OP(builtin_atomic_load),
  AST_OP(builtin_atomic_store),
  AST_OP(builtin_atomic_fetch_add),
  AST_OP(builtin_atomic_fetch_sub),
  AST_OP(builtin_atomic_add_fetch),
  AST_OP(builtin_atomic_sub_fetch),
  AST_OP(builtin_atomic_compare_exchange_bool),
  AST_OP(builtin_atomic_compare_exchange_val),
  AST_OP(builtin_atomic_compare_exchange_n),
  AST_OP(builtin_atomic_fence),
  AST_OP(builtin_source_file),
  AST_OP(builtin_source_line),
  AST_OP(builtin_source_column),
  AST_OP(builtin_source_function),
  AST_OP(builtin_source_pretty_function),

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
  AST_OP(ptr_scale),
  AST_OP(compound_literal),
  AST_OP(stmt_expr),  // GCC statement expression ({ ... }).
  AST_OP(requires_expr),  // C++20 requires-expression.
  AST_OP(static_assert),  // C++11 static_assert declaration.

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
typedef void (*ASTNodePrinter)(struct ASTNode* node, int indents, FILE* fp);
typedef void (*ASTNodeChildReplacer)(struct ASTNode* parent, int child_id,
                                     struct ASTNode* child,
                                     bool delete_old_child);
typedef struct ASTNode* (*ASTNodeCloner)(
    const struct ASTNode* node, struct ASTNode* (*func)(struct ASTNode*, void*),
    void* data);

// Mode passed to visitor function.  It will be called once or twice per
// node.  Prechildren will be used once for every node, even if the node
// has no children.  If the node has children, the function will be called
// again after the children have been processed with PostChildren mode.
typedef enum {
  kVisitPreChildren,   // Called before any children (also if no children)
  kVisitPostChildren,  // Called after chilren.
} VisitorMode;

typedef void (*ASTNodeVisitor)(struct ASTNode* node,
                               void (*func)(struct ASTNode*, void*,
                                            int, VisitorMode),
                               int child_id,
                               void* data);
typedef bool (*ASTNodeUsesValueChecker)(struct ASTNode* node, struct ASTNode* value);

typedef enum {
  kASTTransformContinue,
  kASTTransformSkipChildren,
} ASTNodeTransformAction;

typedef struct ASTNode* (*ASTNodeTransformer)(
    struct ASTNode* node, void* data, ASTNodeTransformAction* action);
typedef void (*ASTNodeTransformVisitor)(struct ASTNode* node,
                                        ASTNodeTransformer func,
                                        void* data);
typedef bool (*ASTNodeUpwardVisitor)(struct ASTNode* node, void* data);

// This is a table of pointers to functions that are provided at runtime
// to implement late-bound functions (a.k.a virtual functions) for an
// AST node.
typedef struct {
  ASTNodeDeleter deleter;  // Function to delete the node.
  ASTNodePrinter printer;  // Function to print the node.
  ASTNodeChildReplacer replacer;
  ASTNodeCloner cloner;
  ASTNodeVisitor visitor;
  ASTNodeUsesValueChecker uses_value;
  ASTNodeTransformVisitor transformer;
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

typedef enum {
  kValueCategoryPrvalue,
  kValueCategoryLvalue,
  kValueCategoryXvalue,
} ASTValueCategory;

typedef struct ASTNode {
  ASTOpcode op;             // Opcode.
  int id;                   // Node id (for debugging).
  int flags;                // Flags
  TypeRecord* type;         // Node type (mostly set by semantic analyzer)
  ASTValueCategory value_category;  // C++ expression value category.
  struct ASTNode* parent;   // Parent node (if any).
  int child_id;             // Which child am I in the parent node?
  SourceLocation location;  // Location in input.
  ASTNodeVirtuals*
      virtuals;  // Virtual table (statically allocated, do not free).
} ASTNode;

struct ConstraintExpr;

// Flags for ASTNode.
#define kASTNeedAddress (1 << 0)     // Need address, not value.
#define kASTStaticInit (1 << 1)      // Static variable init.
#define kASTStatementStart (1 << 2)  // Start of a statement.
#define kASTAnalyzed (1 << 3)        // ASTNode has been analyzed.
#define kASTIsDeclaration (1 << 4)   // This is a declaration.
#define kASTRvoCall (1 << 5)         // Return Value Optimization call.
#define kASTNrvoMarker (1 << 6)      // Named Return Value Optimization symbol.
#define kASTDestructed (1 << 7)      // Node has been destructed (see ASTNodeDelete).
#define kASTLabelUsed (1 << 8)       // Named label has a resolved goto.
#define kASTCoroutineLoweredReturn (1 << 9)  // Return generated by coroutine lowering.
#define kASTCoroutineFrameStore (1 << 10)  // Starter-only coroutine frame store.
#define kASTQualifiedName (1 << 11)  // Identifier was written with :: qualification.
#define kASTCompilerGeneratedGoto (1 << 12)  // Goto synthesized by lowering.
#define kASTPackExpansion (1 << 13)  // Expression is followed by `...`.
#define kASTFoldExpression (1 << 14)  // C++ fold expression placeholder.
#define kASTFoldPackOnLeft (1 << 15)  // Fold pack operand is the left child.
#define kASTDefaultArgument (1 << 16)  // Expression cloned from a default arg.
#define kASTDependentDelete (1 << 17)  // Delete expression parsed before type substitution.
#define kASTDependentArrayDelete (1 << 18)  // Dependent delete[] expression.
#define kASTDependentFunctorCall (1 << 19)  // Dependent object call expression.
#define kASTDependentNewInitializer (1 << 20)  // new T(expr) parsed before T substitution.
#define kASTOverloadDiagnosed (1 << 21)  // Overload-failure diagnostics already emitted for this call.
#define kASTDependentQualifiedName (1 << 22)  // Qualified value name through a dependent (template-parameter) scope.
#define kASTDependentNewValueInit (1 << 23)  // new T() value-init parsed before T substitution.

// Initialize an AST node.
void ASTNodeInit(ASTNode* node, ASTOpcode op, TypeRecord* type,
                 SourceLocation location, ASTNodeVirtuals* virtuals);

// AST nodes are allocated from a bump allocator (see ast.c).  ASTArenaAlloc
// returns zeroed, 16-byte aligned memory.  ASTArenaRelease frees every block at
// once and must only be called after all nodes have been destructed (via
// ASTNodeDelete) so their owned (non-arena) resources are released first.
void* ASTArenaAlloc(size_t size);
void ASTArenaRelease(void);

// Creates a new AST node.  The deleter for this will be ASTNodeBaseDelete
// which simply frees the memory passed.
ASTNode* NewASTNode(ASTOpcode op, TypeRecord* type, SourceLocation location);

// Deletes an AST node based on its runtime type.  It calls the deleter
// function pointer in the node.
void ASTNodeDelete(ASTNode* node);
void ASTNodeSetType(ASTNode* node, TypeRecord* type);
void ASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                         bool delete_old_child);
void ASTNodePrint(ASTNode* node, int indents, FILE* fp);
ASTNode* ASTNodeMove(ASTNode* node);
bool ASTNodeUsesValue(ASTNode* node, ASTNode* value);

// Clone the node and call func with data for every node cloned.
ASTNode* ASTNodeClone(const ASTNode* node, ASTNode* (*func)(ASTNode*, void*),
                      void* data, ASTNode* new_parent);
void ASTNodeVisit(ASTNode* node,
                  void (*func)(ASTNode* node, void*, int, VisitorMode),
                  int child_id,
                  void* data);
void ASTNodeVisitUpwards(ASTNode* node, ASTNodeUpwardVisitor func,
                         void* data);
ASTNode* ASTNodeVisitAndTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data);
ASTNode* ASTNodeVisitAndTransformUpwards(ASTNode* node,
                                         ASTNodeTransformer func,
                                         void* data);
bool ASTNodeIsStatement(ASTNode* node);
bool ASTNodeChildIsStatement(ASTNode* parent, int child_id);

bool ASTNodeIsIntConstant(ASTNode* node);
int64_t ASTNodeConstantValue(ASTNode* node);

// True for call-like nodes (ordinary/inline calls and the variadic/atomic
// builtins) whose arguments live in a child vector starting at child_id 1.
bool ASTIsCallNode(ASTNode* node);

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

typedef struct {
  ASTNode base;
  struct ASTNode* inlined;
  struct ASTNode* ret_value;
} InlineCallASTNode;

ASTNode* NewInlineCallASTNode(TypeRecord* type, SourceLocation location,
                              ASTNode* inlined, ASTNode* ret_value);

// An AST node with a left node and vector of children.
typedef struct {
  ASTNode base;
  ASTNode* left;
  Vector* children;
} VectorASTNode;

ASTNode* NewVectorASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          Vector* children);

typedef struct {
  ASTNode base;
  struct ConstraintExpr* constraint;
} RequiresExpressionASTNode;

ASTNode* NewRequiresExpressionASTNode(struct ConstraintExpr* constraint,
                                      SourceLocation location);

// An identifier node containing a symbol pointer.  The symbol is not deleted
// when the node is deleted.
typedef struct {
  ASTNode base;
  Symbol* symbol;
  Vector* template_arguments;
} IdentifierASTNode;

ASTNode* NewIdentifierASTNode(Symbol* symbol, SourceLocation location);
ASTNode* NewRawIdentifierASTNode(void* symbol, SourceLocation location);

typedef struct {
  ASTNode base;
  StructMember* member;
  CXXAccess access;
  int byte_offset;
  Vector* template_arguments;
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
  Vector* template_arguments;
} ConstantASTNode;

ASTNode* NewIntConstantASTNode(int64_t value, TypeRecord* type,
                               SourceLocation location);
ASTNode* NewRealConstantASTNode(double value, TypeRecord* type,
                                SourceLocation location);
ASTNode* NewStringConstantASTNode(String* value, TypeRecord* type,
                                  SourceLocation location);
ASTNode* NewWideStringConstantASTNode(String* value, TypeRecord* type,
                                      SourceLocation location);
ASTNode* NewCharConstantASTNode(int value, TypeRecord* type,
                                SourceLocation location);

void IntConstantASTNodeInit(ConstantASTNode* node, int64_t value,
                            TypeRecord* type, SourceLocation location);

// AST node holding a type cast of an expression to a particular type.
typedef enum {
  kCastCStyle,
  kCastStatic,
  kCastReinterpret,
  kCastConst,
  kCastDynamic,
} CastKind;

typedef struct {
  ASTNode base;
  TypeRecord* cast_type;
  ASTNode* expr;
  CastKind kind;
  // dynamic_cast that needs a run-time check (polymorphic downcast/sidecast),
  // lowered to a __davecc_dynamic_cast[_ref] call during codegen.
  bool dynamic_runtime;
} CastASTNode;

ASTNode* NewCastASTNode(TypeRecord* type, SourceLocation location,
                        ASTNode* expr);

// Sizeof operation.
typedef struct {
  ConstantASTNode base;
  ASTNode* expr;
  // For `sizeof(type-id)` where the type is dependent on a template parameter,
  // the (refcounted) operand type is retained so its size can be recomputed
  // once the template is instantiated.  NULL for the expression form and for
  // non-dependent type operands (whose size is baked into base.value).
  TypeRecord* type_operand;
  bool is_pack_size;
} SizeofASTNode;

ASTNode* NewSizeofASTNodeWithKnownSize(int size, SourceLocation location);
ASTNode* NewSizeofASTNodeWithExpression(ASTNode* expr, SourceLocation location);
ASTNode* NewSizeofASTNodeWithType(TypeRecord* type, SourceLocation location);
ASTNode* NewSizeofPackASTNode(ASTNode* expr, SourceLocation location);

// typeid operator.  Carries either a type operand (typeid(type-id)) or an
// expression operand (typeid(expr)).  Semantic analysis rewrites this node into
// the underlying type_info access, so it never reaches codegen.
typedef struct {
  ASTNode base;
  ASTNode* expr;            // Expression operand, or NULL for the type form.
  TypeRecord* operand_type; // Type operand for the type form, else NULL.
} TypeidASTNode;

ASTNode* NewTypeidASTNodeWithType(TypeRecord* type, SourceLocation location);
ASTNode* NewTypeidASTNodeWithExpression(ASTNode* expr, SourceLocation location);

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

typedef struct {
  ASTNode base;
  ASTNode* expr;
  String message;
} StaticAssertASTNode;

ASTNode* NewStaticAssertASTNode(ASTNode* expr, String* message,
                                SourceLocation location);

// If statement with condition, if and else parts.  The else part is optional.
typedef struct {
  ASTNode base;
  ASTNode* cond;
  ASTNode* if_part;
  ASTNode* else_part;
  bool is_constexpr;
} IfStatementASTNode;

ASTNode* NewIfStatementASTNode(ASTNode* cond, ASTNode* if_part,
                               ASTNode* else_part, bool is_constexpr,
                               SourceLocation location);

// While and do statements.
// These all contain a condition and a statement.
typedef struct {
  ASTNode base;
  ASTNode* cond;
  ASTNode* stmt;
} CombinedStatementASTNode;

ASTNode* NewCombinedStatementASTNode(ASTOpcode tok, ASTNode* cond,
                                     ASTNode* stmt, SourceLocation location);

typedef struct {
  ASTNode base;
  ASTNode* expr;
} ThrowASTNode;

ASTNode* NewThrowASTNode(ASTNode* expr, SourceLocation location);

// Compound statment, containing a vector of statements.
typedef struct {
  ASTNode base;
  Vector* statements;
  struct LabelASTNode* low_pc;
  struct LabelASTNode* high_pc;
} CompoundStatementASTNode;

ASTNode* NewCompoundStatementASTNode(Vector* statements,
                                     SourceLocation location);
void CompoundASTNodeInsertStatement(CompoundStatementASTNode* node,
                                    ASTNode* stmt, size_t at_index);

typedef struct {
  ASTNode base;
  Symbol* symbol;  // NULL for catch (...).
  ASTNode* stmt;
  bool is_catch_all;
} CatchASTNode;

ASTNode* NewCatchASTNode(Symbol* symbol, bool is_catch_all, ASTNode* stmt,
                         SourceLocation location);

typedef struct {
  ASTNode base;
  ASTNode* try_stmt;
  Vector* catches;  // CatchASTNode*
} TryASTNode;

ASTNode* NewTryASTNode(ASTNode* try_stmt, Vector* catches,
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
  void* saved_sp;        // Used by codegen to store saved SP for VLA.
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
  ASTNode* stmt;
  int64_t value;
  struct IRNode* label;
} CaseLabelASTNode;

ASTNode* NewCaseLabelASTNode(ASTNode* expr, ASTNode* stmt, SourceLocation location);

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
  bool all_cases_covered;   // True if there is no need for a default.
  int max_case_width;       // Max byte width of all case constants.
  bool all_cases_positive;  // All case values are positive.
} SwitchStatementASTNode;

ASTNode* NewSwitchStatementASTNode(ASTNode* expr, ASTNode* stmt,
                                   SourceLocation location);

// Label.
typedef struct LabelASTNode {
  ASTNode base;
  ASTNode* stmt;
  String name;
  struct IRNode* label;
  bool named;     // Use label name in assembly output.
} LabelASTNode;

ASTNode* NewLabelASTNode(const char* name, ASTNode* stmt, bool named, SourceLocation location);

typedef struct {
  String constraint;
  String name;
  ASTNode* expr;
  bool is_output;
  bool is_readwrite;
  bool is_early_clobber;
} AsmOperand;

AsmOperand* NewAsmOperand(const char* constraint, const char* name,
                          ASTNode* expr, bool is_output);
void AsmOperandDelete(AsmOperand* operand);
AsmOperand* AsmOperandClone(AsmOperand* operand,
                            ASTNode* (*func)(ASTNode* node, void*),
                            void* data, ASTNode* new_parent);

typedef struct {
  ASTNode base;
  String* text;
  bool is_volatile;
  bool is_goto;
  Vector outputs;   // AsmOperand*
  Vector inputs;    // AsmOperand*
  Vector clobbers;  // String*
  Vector labels;    // String*
  Vector label_nodes;  // LabelASTNode*, non-owning.
} AsmASTNode;

ASTNode* NewAsmASTNode(String* text, bool is_volatile, SourceLocation location);

// Goto.
typedef struct {
  ASTNode base;
  String* label_name;
  ASTNode* label;  // Not owned, set by semantic analysis.
  ASTNode* lca;    // Not owned, Lowest Common Ancestor with label.
} GotoStatementASTNode;

ASTNode* NewGotoStatementASTNode(String* label_name, SourceLocation location);

// AST node for scaling a pointer by the size of its type.
typedef struct {
  ASTNode base;
  TypeRecord* ref_type;
  ASTOpcode scale_op;     // Either AST_OP(mult) or AST_OP(div)
  ASTNode* expr;
} PtrScaleASTNode;

ASTNode* NewPtrScaleASTNode(TypeRecord* type, ASTOpcode scale_op, ASTNode* expr,
                            SourceLocation location);


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
                                     TypeRecord* type,
                                     SourceLocation location);

// Designated initializer, specifying a struct member or array index and an
// initailizer for it.
typedef enum {
  kDesignatorArray,
  kDesignatorStruct,
  kDesignatorBase,
} DesignatorType;

typedef struct {
  DesignatorType designator_type;
  TypeRecord* type;
  int array_index_end;  // For GCC range designators [start ... end]; == index
                        // when not a range.
  bool is_resolved_member;
  union {
    int array_index;
    String* struct_member_name;   // Before semantic analysis.
    StructMember* struct_member;  // After semantic analysis.
    CXXBaseSpecifier* base;
  } value;
} Designator;

Designator* NewArrayDesignator(TypeRecord* type, int index);
Designator* NewStructDesignator(String* member);
Designator* NewStructMemberDesignator(StructMember* member);
Designator* NewCXXBaseDesignator(CXXBaseSpecifier* base);

typedef struct {
  ASTNode base;
  Vector* designators;
  ASTNode* init;
} DesignatedInitializerASTNode;

ASTNode* NewDesignatedInitializerASTNode(Vector* designators, ASTNode* init,
                                         SourceLocation location);

// Compound literal.
typedef struct {
  ASTNode base;       // base.type is the literal type.
  ASTNode* sym;
  ASTNode* initializer;
} CompoundLiteralASTNode;

ASTNode* NewCompoundLiteralASTNode(ASTNode* sym, SourceLocation location,
                                   ASTNode* initializer);
#endif /* ast_h */
