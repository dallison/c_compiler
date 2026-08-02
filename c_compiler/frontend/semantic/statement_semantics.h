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

#endif /* statement_semantics_h */
