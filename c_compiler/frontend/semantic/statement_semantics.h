//
//  statement_semantics.h
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef statement_semantics_h
#define statement_semantics_h

#include "semantics.h"
#include "syntax.h"

void AnalyzeStatement(ASTNode* node);
void CheckUnusedLabels(ASTNode* body);
ASTNode* AppendCXXFullExpressionTemporaryDestructors(ASTNode* expr);

// Materializes a structured binding retained in a dependent template body.
// Fixed binding symbols are recorded in symbol_map and a structured-binding
// pack is recorded as its concrete element-symbol vector in pack_symbol_map.
// Takes ownership of node and returns an analyzed declaration-list statement.
ASTNode* SemanticMaterializeClonedStructuredBinding(
    ASTNode* node, Map* symbol_map, Map* pack_symbol_map);

// Completes return type deduction for a function whose body has been analyzed
// but whose placeholder return type is still unresolved.  A body with no
// operand-carrying `return` deduces as if from `return;` at the closing brace
// ([dcl.spec.auto]/8); otherwise the recorded return operands are retried.
// Leaves the placeholder in place when the operands are still type-dependent.
void StatementFinishAutoReturnDeduction(TypeRecord* func,
                                        ASTNode* diagnostic_node);

// Inserts C++ scope-exit destructor calls for automatic objects at each
// return/break/continue in a fully-analyzed, non-dependent function body.  Must
// run with compiler->current_function (and access context) set to `func`.
void CXXInsertScopeExitDestructors(TypeRecord* func);

// Appends the temporary symbols whose lifetimes are extended by a synthesized
// range-for initializer, in destruction order.
void CXXCollectRangeForInitializerTemporaries(ASTNode* range_decl,
                                              Vector* out);

typedef struct {
  size_t element_count;
  bool tuple_like;
  bool array_like;
  Vector members;  // StructMember* when decomposed from a class type.
} StructuredBindingDecomposition;

// Fills decomposition metadata for a structured-binding source type.
bool SemanticAnalyzeStructuredBindingDecomposition(TypeRecord* type,
                                                   ASTNode* diagnostic,
                                                   StructuredBindingDecomposition* out);
void SemanticStructuredBindingDecompositionDestruct(
    StructuredBindingDecomposition* decomposition);

// Element access for hidden structured-binding decomposition ([dcl.struct.bind]).
ASTNode* SemanticStructuredBindingElementAccess(
    Symbol* hidden, TypeRecord* hidden_type, size_t index,
    StructMember* member, bool tuple_like, SourceLocation location);

// Clone a local symbol for another expansion iteration, preserving spelling.
Symbol* SemanticCloneExpansionIterationSymbol(Symbol* source);

// Hidden `auto&&` binding for an expansion initializer, with reference
// extension for lvalues.  Returns the hidden-declaration statement.
ASTNode* SemanticCreateHiddenReferenceBinding(ASTNode* init_expr,
                                              SourceLocation location,
                                              Symbol** hidden_out);

void SemanticMarkExpansionLoopJumps(ExpansionStatementASTNode* expansion);
void SemanticDiagnoseExpansionEnclosedLabels(ExpansionStatementASTNode* expansion);

void SemanticAppendHiddenInitializerTemporaries(ASTNode* hidden_decl,
                                                Vector* statements);

#endif /* statement_semantics_h */
