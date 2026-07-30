//
//  statement_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_semantics.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "compiler.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "init_semantics.h"
#include "bitset.h"
#include "errors.h"
#include "type_inheritance.h"

static ASTNode* StaticAssertIdentityClone(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static void ClearStaticAssertAnalysis(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

static bool ExpressionHasSideEffects(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(call):
    case AST_OP(inline_call):
    case AST_OP(asm):
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
      return true;
    case AST_OP(comma): {
      BinaryASTNode* n = (BinaryASTNode*)node;
      return ExpressionHasSideEffects(n->left) || ExpressionHasSideEffects(n->right);
    }
    case AST_OP(question): {
      BinaryASTNode* n = (BinaryASTNode*)node;
      BinaryASTNode* arms = (BinaryASTNode*)n->right;
      return ExpressionHasSideEffects(n->left) ||
             (arms != NULL &&
              (ExpressionHasSideEffects(arms->left) ||
               ExpressionHasSideEffects(arms->right)));
    }
    default:
      return false;
  }
}

static bool VectorContainsPointer(Vector* vector, void* ptr) {
  for (size_t i = 0; vector != NULL && i < vector->length; i++) {
    if (vector->value.p[i] == ptr) {
      return true;
    }
  }
  return false;
}

static bool CXXTemporaryNeedsDestructor(Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || !sym->flags.is_temp ||
      sym->type == NULL || !TypeIsStructOrUnion(sym->type) ||
      sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->tag_name == NULL) {
    return false;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, sym->type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(sym->type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  return destructor != NULL && destructor->is_member_function &&
         destructor->symbol != NULL && destructor->symbol->type != NULL &&
         destructor->symbol->type->info.function.is_destructor &&
         !(destructor->symbol->type->info.function.is_implicitly_declared &&
           destructor->symbol->type->info.function.is_trivial_special_member);
}

static bool CXXSameClassIgnoringQualifiers(TypeRecord* left,
                                           TypeRecord* right) {
  return TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
         left->info.struct_info == right->info.struct_info;
}

static ASTNode* CXXSingleExpressionInitializer(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer->op != AST_OP(braced_init)) {
    return NULL;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (braced->initializers == NULL || braced->initializers->length != 1) {
    return NULL;
  }
  ASTNode* only = braced->initializers->value.p[0];
  if (only != NULL && only->op == AST_OP(designated_init)) {
    only = ((DesignatedInitializerASTNode*)only)->init;
  }
  if (only != NULL && only->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)only)->expr;
  }
  return only;
}

static Symbol* CXXTemporaryConstructionResultSymbol(ASTNode* expr) {
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(identifier)) {
    Symbol* sym = ((IdentifierASTNode*)expr)->symbol;
    return sym != NULL && sym->flags.is_temp ? sym : NULL;
  }
  if (expr->op == AST_OP(comma)) {
    return CXXTemporaryConstructionResultSymbol(((BinaryASTNode*)expr)->right);
  }
  if (expr->op == AST_OP(cast)) {
    return CXXTemporaryConstructionResultSymbol(((CastASTNode*)expr)->expr);
  }
  return NULL;
}

static bool CXXCompoundLiteralWrapsConstructedTemporary(
    CompoundLiteralASTNode* literal) {
  if (literal == NULL || literal->sym == NULL ||
      literal->sym->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* literal_sym = ((IdentifierASTNode*)literal->sym)->symbol;
  ASTNode* expr = CXXSingleExpressionInitializer(literal->initializer);
  Symbol* source = CXXTemporaryConstructionResultSymbol(expr);
  return literal_sym != NULL && source != NULL && literal_sym != source &&
         CXXSameClassIgnoringQualifiers(literal_sym->type, source->type);
}

static bool CXXTemporaryIsElidedByDirectInitialization(ASTNode* node,
                                                       Symbol* sym) {
  for (ASTNode* parent = node != NULL ? node->parent : NULL; parent != NULL;
       parent = parent->parent) {
    if (parent->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)parent;
      ASTNode* expr = designated->init;
      if (expr != NULL && expr->op == AST_OP(expr_init)) {
        expr = ((ExpressionInitializerASTNode*)expr)->expr;
      }
      Designator* first_designator =
          designated->designators != NULL &&
                  designated->designators->length != 0
              ? designated->designators->value.p[0]
              : NULL;
      if (first_designator != NULL &&
          first_designator->designator_type == kDesignatorArray &&
          CXXTemporaryConstructionResultSymbol(expr) == sym &&
          CXXSameClassIgnoringQualifiers(parent->type, sym->type)) {
        return true;
      }
    }
  }
  return false;
}

// Mirror of the backend's CXXDesignatedInitFunctionalCastConstructor
// (expr_codegen.c): does this initializer expression construct its result in
// place via a functional-cast / converting constructor call (possibly reached
// through an expr_init wrapper or a comma)?  When true, the backend elides the
// constructor directly into the destination storage rather than building a
// separate object and copying it in.
static bool CXXInitializerConstructsInPlace(ASTNode* init) {
  if (init == NULL) {
    return false;
  }
  if (init->op == AST_OP(expr_init)) {
    return CXXInitializerConstructsInPlace(
        ((ExpressionInitializerASTNode*)init)->expr);
  }
  if (init->op == AST_OP(call)) {
    ASTNode* callee = ((VectorASTNode*)init)->left;
    return callee != NULL && callee->type != NULL &&
           TypeIsFunction(callee->type) &&
           callee->type->info.function.is_constructor;
  }
  if (init->op == AST_OP(comma)) {
    return CXXInitializerConstructsInPlace(((BinaryASTNode*)init)->left);
  }
  return false;
}

// True when the backend elides the compound literal's construction directly
// into the compound literal's own storage.  In that case the compound literal's
// temporary is the live object (and must be destroyed), while the inner
// construction-source temporary is never materialized on its own (and must be
// skipped via the `elided` set in CXXTemporaryCollection).
static bool CXXCompoundLiteralElidesIntoOwnStorage(
    CompoundLiteralASTNode* literal) {
  if (!CXXCompoundLiteralWrapsConstructedTemporary(literal)) {
    return false;
  }
  return CXXInitializerConstructsInPlace(
      CXXSingleExpressionInitializer(literal->initializer));
}

// State threaded through CollectCXXTemporarySymbols: `temps` are the temporaries
// to destroy at the end of the full expression; `elided` are inner
// construction-source temporaries that a compound literal elides into its own
// storage and that therefore must not be destroyed on their own.  The AST is
// visited pre-order, so a compound literal is always seen before the inner
// temporary nested in its initializer.
typedef struct {
  Vector temps;
  Vector elided;
} CXXTemporaryCollection;

static void CollectCXXTemporarySymbols(ASTNode* node, void* data, int child_id,
                                       VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  CXXTemporaryCollection* collection = data;
  Symbol* sym = NULL;
  if (node->op == AST_OP(identifier)) {
    if (node->parent != NULL && node->parent->op == AST_OP(compound_literal) &&
        child_id == 0 &&
        CXXCompoundLiteralWrapsConstructedTemporary(
            (CompoundLiteralASTNode*)node->parent)) {
      return;
    }
    sym = ((IdentifierASTNode*)node)->symbol;
    if (sym != NULL && CXXTemporaryIsElidedByDirectInitialization(node, sym)) {
      return;
    }
    // An inner temporary that a compound literal elides into its own storage is
    // never a live object; destroying it would run a destructor on
    // uninitialized storage.  The enclosing compound literal's temporary is
    // destroyed instead (collected below when the compound literal is visited).
    if (sym != NULL && VectorContainsPointer(&collection->elided, sym)) {
      return;
    }
  } else if (node->op == AST_OP(compound_literal)) {
    CompoundLiteralASTNode* literal = (CompoundLiteralASTNode*)node;
    // A compound literal that only wraps the construction of an inner temporary
    // does not own storage of its own -- unless the backend elides the
    // constructor directly into the compound literal's own slot, in which case
    // the compound literal's temporary is the live object that must be
    // destroyed and the inner source temporary must be skipped.
    if (CXXCompoundLiteralWrapsConstructedTemporary(literal)) {
      if (!CXXCompoundLiteralElidesIntoOwnStorage(literal)) {
        return;
      }
      Symbol* source = CXXTemporaryConstructionResultSymbol(
          CXXSingleExpressionInitializer(literal->initializer));
      if (source != NULL && !VectorContainsPointer(&collection->elided, source)) {
        VectorAppend(&collection->elided, source);
      }
    }
    if (literal->sym != NULL && literal->sym->op == AST_OP(identifier)) {
      sym = ((IdentifierASTNode*)literal->sym)->symbol;
    }
  } else {
    return;
  }
  if (CXXTemporaryNeedsDestructor(sym) &&
      !VectorContainsPointer(&collection->temps, sym)) {
    VectorAppend(&collection->temps, sym);
  }
}

static ASTNode* NewCXXTemporaryDestructorCall(Symbol* sym,
                                             SourceLocation location) {
  if (!CXXTemporaryNeedsDestructor(sym)) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, sym->type->info.struct_info->tag_name);
  ASTNode* receiver = NewIdentifierASTNode(sym, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, NewVector());
  return AnalyzeExpression(call);
}

static ASTNode* AppendCXXFullExpressionTemporaryDestructors(ASTNode* expr) {
  if (!CompilerIsCXX() || expr == NULL ||
      (compiler->current_function != NULL &&
       (compiler->current_function->info.function.is_coroutine ||
        compiler->current_function->info.function.coroutine_frame_type != NULL))) {
    return expr;
  }
  CXXTemporaryCollection collection;
  VectorInit(&collection.temps);
  VectorInit(&collection.elided);
  ASTNodeVisit(expr, CollectCXXTemporarySymbols, 0, &collection);
  for (size_t i = collection.temps.length; i > 0; i--) {
    Symbol* sym = collection.temps.value.p[i - 1];
    ASTNode* destructor =
        NewCXXTemporaryDestructorCall(sym, expr->location);
    if (destructor == NULL) {
      continue;
    }
    expr = NewBinaryASTNode(AST_OP(comma), destructor->type,
                            expr->location, expr, destructor);
    expr->flags |= kASTAnalyzed;
  }
  VectorDestruct(&collection.temps);
  VectorDestruct(&collection.elided);
  return expr;
}

// ---- C++ scope-exit destructor insertion ---------------------------------
//
// A block-scope object with a non-trivial destructor is destroyed when its
// block exits by falling off the end -- the parser appends the destructor call
// as a trailing statement (SyntaxAppendCXXBlockScopeDestructors).  A jump out of
// the block (return/break/continue) branches past those trailing statements, so
// this pass injects the required destructor calls into each jump.
//
// For `return` the destructors must run *after* the return value is
// materialised (the value may name the objects, and a by-value returned local
// must be copied to the result before it is destroyed), so they are hung off
// the return node's otherwise-unused `stmt` child and emitted by the backend at
// the correct point.  For `break`/`continue` there is no value, so the jump is
// rewritten as `{ <destructors>; jump; }`.

// Build and analyze `receiver.~Tag()` as an expression statement.
static ASTNode* NewAnalyzedDestructorStatement(TypeRecord* type,
                                               ASTNode* receiver,
                                               SourceLocation location) {
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  ASTNode* member = NewStringConstantASTNode(NewString(destructor_name.value),
                                             NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  // A class with virtual bases has a destructor with a hidden complete-object
  // flag; a named local is a complete (most-derived) object, so pass 1.
  Vector* actuals = NewVector();
  if (StructHasVirtualBases(type->info.struct_info)) {
    VectorAppend(actuals,
                 NewIntConstantASTNode(
                     1, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  }
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  call = AnalyzeExpression(call);
  ASTNode* statement = NewExpressionStatementASTNode(call, location);
  statement->flags |= kASTAnalyzed;
  return statement;
}

// Is `sym` an automatic object this pass is responsible for destroying?
static bool CXXLocalNeedsScopeExitDestructor(Symbol* sym) {
  if (sym == NULL || sym->flags.is_temp ||
      StorageIs(sym->storage, STO(static)) ||
      StorageIs(sym->storage, STO(extern))) {
    return false;
  }
  return TypeHasNonTrivialDestructor(sym->type);
}

// Append destructor statement(s) for `sym` (a single object, or a fixed array
// of objects destroyed in reverse index order) to `out`.
static void AppendLocalDestructorStatements(Symbol* sym, Vector* out) {
  SourceLocation location = sym->location;
  if (TypeIsFixedArray(sym->type)) {
    int64_t length = sym->type->info.array.size.fixed;
    for (int64_t k = length; k > 0; k--) {
      ASTNode* array = NewIdentifierASTNode(sym, location);
      ASTNode* subscript = NewBinaryASTNode(
          AST_OP(subscript), NULL, location, array,
          NewIntConstantASTNode(k - 1,
                                NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location));
      VectorAppend(out, NewAnalyzedDestructorStatement(sym->type->next,
                                                       subscript, location));
    }
    return;
  }
  VectorAppend(out, NewAnalyzedDestructorStatement(
                        sym->type, NewIdentifierASTNode(sym, location),
                        location));
}

static size_t IndexOfChildInCompound(CompoundStatementASTNode* compound,
                                     ASTNode* child) {
  for (size_t i = 0; i < compound->statements->length; i++) {
    if (compound->statements->value.p[i] == child) {
      return i;
    }
  }
  return compound->statements->length;
}

// Append (to `out`, in reverse construction order) the destructor statements
// for the automatic objects declared directly in `compound` at statement
// indices [lo, hi).  `skip` (an NRVO'd returned object) is never destroyed.
static void CollectCompoundLocalDestructors(CompoundStatementASTNode* compound,
                                            size_t lo, size_t hi, Symbol* skip,
                                            Vector* out) {
  if (hi > compound->statements->length) {
    hi = compound->statements->length;
  }
  for (size_t i = hi; i > lo; i--) {
    ASTNode* stmt = compound->statements->value.p[i - 1];
    if (stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
    for (size_t j = decls->declarations->length; j > 0; j--) {
      ASTNode* decl_node = decls->declarations->value.p[j - 1];
      if (decl_node->op != AST_OP(vardecl)) {
        continue;
      }
      Symbol* sym = ((VariableDeclarationASTNode*)decl_node)->symbol;
      if (sym == skip || !CXXLocalNeedsScopeExitDestructor(sym)) {
        continue;
      }
      AppendLocalDestructorStatements(sym, out);
    }
  }
}

// Collect (into `out`, in destruction order) the destructor statements for the
// automatic objects that go out of scope when control leaves `jump`.  Walks
// enclosing compound statements from innermost outward, stopping once `limit`
// has been reached: for a `return`, `limit` is the function-body compound (which
// is itself processed); for `break`/`continue`, `limit` is the enclosing
// loop/switch node (not a compound, so processing stops just inside it).  Only
// objects declared before the path to the jump in each compound are live, and
// `skip` (an NRVO'd returned object) is never destroyed.
static void CollectScopeExitDestructors(ASTNode* jump, ASTNode* limit,
                                        Symbol* skip, Vector* out) {
  ASTNode* child = jump;
  ASTNode* parent = jump->parent;
  while (parent != NULL) {
    if (parent->op == AST_OP(compound)) {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)parent;
      size_t idx = IndexOfChildInCompound(compound, child);
      CollectCompoundLocalDestructors(compound, 0, idx, skip, out);
    }
    if (parent == limit) {
      break;
    }
    child = parent;
    parent = parent->parent;
  }
}

static ASTNode* CXXEnclosingLoopOrSwitch(ASTNode* node) {
  for (ASTNode* p = node->parent; p != NULL; p = p->parent) {
    if (p->op == AST_OP(for) || p->op == AST_OP(while) ||
        p->op == AST_OP(do) || p->op == AST_OP(switch)) {
      return p;
    }
  }
  return NULL;
}

static ASTNode* CXXEnclosingLoop(ASTNode* node) {
  for (ASTNode* p = node->parent; p != NULL; p = p->parent) {
    if (p->op == AST_OP(for) || p->op == AST_OP(while) ||
        p->op == AST_OP(do)) {
      return p;
    }
  }
  return NULL;
}

static void ProcessReturnJump(TypeRecord* func,
                              CombinedStatementASTNode* ret) {
  if ((ret->base.flags & kASTScopeExitCleanup) != 0) {
    return;
  }
  ret->base.flags |= kASTScopeExitCleanup;
  Symbol* skip = NULL;
  ASTNode* return_value = ret->cond;
  if (return_value != NULL && return_value->op == AST_OP(identifier) &&
      (return_value->flags & kASTNrvoMarker) != 0) {
    skip = ((IdentifierASTNode*)return_value)->symbol;
  }
  Vector* destructors = NewVector();
  CollectScopeExitDestructors((ASTNode*)ret, func->info.function.body, skip,
                              destructors);
  if (destructors->length == 0) {
    VectorDelete(destructors);
    return;
  }
  ASTNode* compound =
      NewCompoundStatementASTNode(destructors, ret->base.location);
  compound->flags |= kASTAnalyzed;
  ret->stmt = compound;
  compound->parent = (ASTNode*)ret;
  compound->child_id = 1;
}

static void ProcessBreakContinueJump(ASTNode* jump) {
  if ((jump->flags & kASTScopeExitCleanup) != 0) {
    return;
  }
  jump->flags |= kASTScopeExitCleanup;
  ASTNode* limit = jump->op == AST_OP(break) ? CXXEnclosingLoopOrSwitch(jump)
                                             : CXXEnclosingLoop(jump);
  if (limit == NULL) {
    return;
  }
  Vector* destructors = NewVector();
  CollectScopeExitDestructors(jump, limit, NULL, destructors);
  if (destructors->length == 0) {
    VectorDelete(destructors);
    return;
  }
  ASTNode* parent = jump->parent;
  int child_id = jump->child_id;
  // The jump runs after the destructors; NewCompoundStatementASTNode reparents
  // it into the new block, then we splice the block into the jump's old slot.
  VectorAppend(destructors, jump);
  ASTNode* compound = NewCompoundStatementASTNode(destructors, jump->location);
  compound->flags |= kASTAnalyzed;
  ASTNodeReplaceChild(parent, child_id, compound, false);
}

// Returns the ancestor of `descendant` whose parent is `ancestor` (i.e. the
// child of `ancestor` on the path down to `descendant`), or NULL if `ancestor`
// is not on the parent chain.
static ASTNode* CXXChildTowards(ASTNode* descendant, ASTNode* ancestor) {
  ASTNode* node = descendant;
  while (node != NULL && node->parent != ancestor) {
    node = node->parent;
  }
  return node;
}

// Inserts scope-exit destructor calls for a `goto` (rewriting it as
// `{ <destructors>; goto; }`).  A goto destroys every automatic object whose
// scope it exits: all objects declared before it in each enclosing block down
// to the least common ancestor (LCA) of the goto and its target label.  Within
// the LCA block itself only a *backward* jump exits scopes -- the objects
// declared after the label but before the goto are destroyed (they are
// reconstructed when control re-enters); a forward jump instead enters those
// scopes, so nothing there is destroyed (jumping past a non-trivial
// initialization is already rejected by CheckJumpBypassedDeclarations).
static void ProcessGotoJump(GotoStatementASTNode* go) {
  ASTNode* jump = &go->base;
  if ((jump->flags & kASTScopeExitCleanup) != 0) {
    return;
  }
  jump->flags |= kASTScopeExitCleanup;
  // Compiler-lowered gotos (e.g. coroutine lowering) manage object lifetimes
  // themselves and must not get a second set of destructor calls.
  if ((jump->flags & kASTCompilerGeneratedGoto) != 0 || go->label == NULL ||
      go->lca == NULL) {
    return;
  }

  Vector* destructors = NewVector();
  // Scopes strictly between the goto and the LCA are fully exited.
  ASTNode* child = jump;
  ASTNode* parent = jump->parent;
  while (parent != NULL && parent != go->lca) {
    if (parent->op == AST_OP(compound)) {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)parent;
      size_t idx = IndexOfChildInCompound(compound, child);
      CollectCompoundLocalDestructors(compound, 0, idx, NULL, destructors);
    }
    child = parent;
    parent = parent->parent;
  }
  // The LCA block is only partially exited, and only on a backward jump.
  if (parent == go->lca && go->lca->op == AST_OP(compound)) {
    CompoundStatementASTNode* compound = (CompoundStatementASTNode*)go->lca;
    ASTNode* label_branch = CXXChildTowards(go->label, go->lca);
    if (label_branch != NULL) {
      size_t jump_idx = IndexOfChildInCompound(compound, child);
      size_t label_idx = IndexOfChildInCompound(compound, label_branch);
      if (jump_idx > label_idx) {
        CollectCompoundLocalDestructors(compound, label_idx + 1, jump_idx, NULL,
                                        destructors);
      }
    }
  }

  if (destructors->length == 0) {
    VectorDelete(destructors);
    return;
  }
  ASTNode* jump_parent = jump->parent;
  int child_id = jump->child_id;
  // The goto runs after the destructors; NewCompoundStatementASTNode reparents
  // it into the new block, then we splice the block into the goto's old slot.
  VectorAppend(destructors, jump);
  ASTNode* compound = NewCompoundStatementASTNode(destructors, jump->location);
  compound->flags |= kASTAnalyzed;
  ASTNodeReplaceChild(jump_parent, child_id, compound, false);
}

static void CollectJumpStatements(ASTNode* node, void* data, int child_id,
                                  VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if (node->op == AST_OP(return) || node->op == AST_OP(break) ||
      node->op == AST_OP(continue) || node->op == AST_OP(goto)) {
    VectorAppend((Vector*)data, node);
  }
}

static void RebuildCompoundFallthroughDestructors(ASTNode* node, void* data,
                                                  int child_id,
                                                  VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(compound)) {
    return;
  }
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)node;
  for (size_t i = compound->statements->length; i > 0; i--) {
    ASTNode* statement = compound->statements->value.p[i - 1];
    if ((statement->flags & kASTFallthroughDestructor) == 0) {
      continue;
    }
    VectorDeleteElement(compound->statements, i - 1);
    ASTNodeDelete(statement);
  }

  Vector destructors;
  VectorInit(&destructors);
  CollectCompoundLocalDestructors(compound, 0, compound->statements->length,
                                  NULL, &destructors);
  for (size_t i = 0; i < destructors.length; i++) {
    ASTNode* destructor = destructors.value.p[i];
    destructor->flags |= kASTFallthroughDestructor;
    VectorAppend(compound->statements, destructor);
  }
  VectorDestruct(&destructors);
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* statement = compound->statements->value.p[i];
    statement->parent = node;
    statement->child_id = (int)i;
  }
}

// Inserts scope-exit destructor calls for automatic objects at every
// return/break/continue in `func`'s body (see the block comment above).
void CXXInsertScopeExitDestructors(TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      func->info.function.body == NULL ||
      func->info.function.is_coroutine ||
      func->info.function.coroutine_frame_type != NULL ||
      TypeContainsTemplateParameter(func)) {
    return;
  }
  ASTNodeVisit(func->info.function.body,
               RebuildCompoundFallthroughDestructors, 0, NULL);
  Vector jumps;
  VectorInit(&jumps);
  ASTNodeVisit(func->info.function.body, CollectJumpStatements, 0, &jumps);
  for (size_t i = 0; i < jumps.length; i++) {
    ASTNode* jump = jumps.value.p[i];
    if (jump->op == AST_OP(return)) {
      ProcessReturnJump(func, (CombinedStatementASTNode*)jump);
    } else if (jump->op == AST_OP(goto)) {
      ProcessGotoJump((GotoStatementASTNode*)jump);
    } else {
      ProcessBreakContinueJump(jump);
    }
  }
  VectorDestruct(&jumps);
}

static void AnalyzeExpressionStatement(ExpressionStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  node->expr = AppendCXXFullExpressionTemporaryDestructors(node->expr);

  // warn_unused_result: a discarded call to a function so annotated.
  ASTNode* expr = node->expr;
  if (expr != NULL && expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)expr;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
      if (callee != NULL && SymbolHasAttribute(callee, "warn_unused_result")) {
        SemanticWarning(expr, "unused-result",
                        "ignoring return value of '%s' declared with "
                        "warn_unused_result",
                        callee->name.value);
      }
    }
  }
  if (expr != NULL && expr->type != NULL && !TypeIsVoid(expr->type) &&
      !ExpressionHasSideEffects(expr)) {
    SemanticWarning(expr, "unused-value", "expression result unused");
  }
}

static void AnalyzeStaticAssert(StaticAssertASTNode* node) {
  ASTNode* expr = ASTNodeClone(node->expr, StaticAssertIdentityClone, NULL, NULL);
  if (expr == NULL) {
    SemanticError((ASTNode*)node, "Invalid static_assert expression");
    return;
  }
  ASTNodeVisit(expr, ClearStaticAssertAnalysis, 0, NULL);
  expr = AnalyzeExpression(expr);
  if (expr == NULL) {
    ASTNodeDelete(expr);
    SemanticError((ASTNode*)node,
                  "static_assert expression is not an integer constant expression");
    return;
  }
  int64_t value = 0;
  if (!EvaluateIntegerExpression(expr, &value)) {
    ASTNodeDelete(expr);
    SemanticError((ASTNode*)node,
                  "static_assert expression is not an integer constant expression");
    return;
  }
  ASTNodeDelete(expr);
  if (value == 0) {
    SemanticError((ASTNode*)node, "%s", node->message.value);
  }
}

static bool AsmOutputHasAddress(ASTNode* node) {
  if (node == NULL || TypeIsConst(node->type) || TypeIsFunction(node->type) ||
      TypeIsArray(node->type)) {
    return false;
  }
  switch (node->op) {
    case AST_OP(identifier):
    case AST_OP(subscript):
    case AST_OP(contents):
    case AST_OP(dot):
    case AST_OP(arrow):
    case AST_OP(cast):
    case AST_OP(compound_literal):
      return true;
    default:
      return false;
  }
}

static bool IsAArch64RegisterClobber(const char* name) {
  if (name[0] == 'x' || name[0] == 'w' || name[0] == 'd' || name[0] == 's' ||
      name[0] == 'v') {
    char* end = NULL;
    long reg = strtol(name + 1, &end, 10);
    return end != name + 1 && *end == '\0' && reg >= 0 && reg <= 31;
  }
  return strcmp(name, "lr") == 0 || strcmp(name, "sp") == 0 ||
         strcmp(name, "xzr") == 0 || strcmp(name, "wzr") == 0;
}

static bool IsNumericRegisterClobber(const char* name, const char* prefix,
                                     long max_reg) {
  size_t prefix_len = strlen(prefix);
  if (strncmp(name, prefix, prefix_len) != 0) {
    return false;
  }
  char* end = NULL;
  long reg = strtol(name + prefix_len, &end, 10);
  return end != name + prefix_len && *end == '\0' && reg >= 0 && reg <= max_reg;
}

static const char* AsmTargetName(void) {
  if (compiler->target == NULL) {
    return "target";
  }
  return compiler->target->name.value;
}

static bool IsSupportedAsmConstraint(const char* constraint, bool is_output) {
  const char* target = AsmTargetName();
  if (strcmp(target, "6502") == 0) {
    bool saw_constraint = false;
    for (const char* p = constraint; *p != '\0'; p++) {
      switch (*p) {
        case '=':
        case '+':
        case '&':
          break;
        case 'r':
        case 'i':
        case 'g':
          saw_constraint = true;
          break;
        default:
          return false;
      }
    }
    if (is_output) {
      return saw_constraint && (constraint[0] == '=' || constraint[0] == '+');
    }
    return saw_constraint;
  }
  bool saw_constraint = false;
  for (const char* p = constraint; *p != '\0'; p++) {
    switch (*p) {
      case '=':
      case '+':
      case '&':
      case '%':
        break;
      case 'r':
      case 'm':
      case 'i':
      case 'g':
        saw_constraint = true;
        break;
      case 'w':
        if (strcmp(target, "aarch64") != 0) {
          return false;
        }
        saw_constraint = true;
        break;
      default:
        return false;
    }
  }
  if (is_output) {
    return saw_constraint && (constraint[0] == '=' || constraint[0] == '+');
  }
  return saw_constraint;
}

static bool IsSupportedAsmClobber(const char* name) {
  const char* target = AsmTargetName();
  if (strcmp(name, "memory") == 0 || strcmp(name, "cc") == 0) {
    return true;
  }
  if (strcmp(target, "aarch64") == 0) {
    return IsAArch64RegisterClobber(name);
  }
  if (strcmp(target, "arm") == 0) {
    return IsNumericRegisterClobber(name, "r", 15) || strcmp(name, "lr") == 0 ||
           strcmp(name, "sp") == 0 || strcmp(name, "pc") == 0;
  }
  if (strcmp(target, "riscv") == 0) {
    return IsNumericRegisterClobber(name, "x", 31) ||
           IsNumericRegisterClobber(name, "f", 31) ||
           strcmp(name, "ra") == 0 || strcmp(name, "sp") == 0 ||
           strcmp(name, "fp") == 0;
  }
  if (strcmp(target, "x86_64") == 0 || strcmp(target, "x86-64") == 0) {
    static const char* regs[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    for (size_t i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
      if (strcmp(name, regs[i]) == 0) {
        return true;
      }
    }
    return strncmp(name, "xmm", 3) == 0 && IsNumericRegisterClobber(name, "xmm", 15);
  }
  return strcmp(target, "6502") == 0;
}

typedef struct {
  String* label_name;
  ASTNode* label;
} AsmLabelFinder;

static void FindAsmLabel(ASTNode* node, void* data, int child_id,
                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  AsmLabelFinder* finder = data;
  if (finder->label != NULL) {
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(finder->label_name, &label->name)) {
      finder->label = node;
    }
  }
}

static void AnalyzeAsmStatement(AsmASTNode* node) {
  if (node->outputs.length + node->inputs.length > 16) {
    SemanticError(&node->base, "Too many asm operands");
  }
  for (size_t i = 0; i < node->outputs.length; i++) {
    AsmOperand* operand = node->outputs.value.p[i];
    operand->expr = AnalyzeExpression(operand->expr);
    if (!AsmOutputHasAddress(operand->expr)) {
      SemanticError(operand->expr, "asm output operand must be an assignable lvalue");
    }
    if (!IsSupportedAsmConstraint(operand->constraint.value, true)) {
      SemanticError(&node->base, "Unsupported %s asm output constraint '%s'",
                    AsmTargetName(), operand->constraint.value);
    }
  }
  for (size_t i = 0; i < node->inputs.length; i++) {
    AsmOperand* operand = node->inputs.value.p[i];
    operand->expr = AnalyzeExpression(operand->expr);
    if (!IsSupportedAsmConstraint(operand->constraint.value, false)) {
      SemanticError(&node->base, "Unsupported %s asm input constraint '%s'",
                    AsmTargetName(), operand->constraint.value);
    }
  }
  for (size_t i = 0; i < node->clobbers.length; i++) {
    String* clobber = node->clobbers.value.p[i];
    if (!IsSupportedAsmClobber(clobber->value)) {
      SemanticError(&node->base, "Unsupported %s asm clobber '%s'",
                    AsmTargetName(), clobber->value);
    }
  }
  for (size_t i = 0; i < node->labels.length; i++) {
    String* label_name = node->labels.value.p[i];
    AsmLabelFinder finder = {.label_name = label_name, .label = NULL};
    ASTNodeVisit(compiler->current_function->info.function.body, FindAsmLabel, 0,
                 &finder);
    if (finder.label == NULL) {
      SemanticError(&node->base, "Undefined asm goto label '%s'",
                    label_name->value);
    } else {
      finder.label->flags |= kASTLabelUsed;
      ((LabelASTNode*)finder.label)->named = true;
      VectorAppend(&node->label_nodes, finder.label);
    }
  }
}

static void AnalyzeIfStatement(IfStatementASTNode* node) {
  if (node->is_consteval) {
    node->cond = AnalyzeExpression(node->cond);
    if (node->consteval_negated) {
      AnalyzeStatement(node->if_part);
      compiler->immediate_function_context_depth++;
      AnalyzeStatement(node->else_part);
      compiler->immediate_function_context_depth--;
    } else {
      compiler->immediate_function_context_depth++;
      AnalyzeStatement(node->if_part);
      compiler->immediate_function_context_depth--;
      AnalyzeStatement(node->else_part);
    }
    return;
  }
  node->cond = AnalyzeExpression(node->cond);
  SemanticConvertType(node->cond, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  SemanticCheckScalarType(node->cond);
  if (node->is_constexpr) {
    int64_t value;
    if (!EvaluateIntegerExpression(node->cond, &value)) {
      SemanticError(node->cond,
                    "if constexpr condition is not a constant expression");
      return;
    }
    ASTNode* selected = value != 0 ? node->if_part : node->else_part;
    if (selected == NULL) {
      selected = NewCompoundStatementASTNode(NewVector(), node->base.location);
    }
    AnalyzeStatement(selected);
    node->cond = NewIntConstantASTNode(
        1, NewTypeRecordWithSize(kTypeBool, kQualPlain), node->base.location);
    node->cond->parent = (ASTNode*)node;
    node->cond->child_id = 0;
    node->if_part = selected;
    node->if_part->parent = (ASTNode*)node;
    node->if_part->child_id = 1;
    node->else_part = NULL;
    SemanticCheckScalarType(node->cond);
    return;
  }
  AnalyzeStatement(node->if_part);
  AnalyzeStatement(node->else_part);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeWhileStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticConvertType(node->cond, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeDoStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticConvertType(node->cond, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

// Function to compare case values for the qsort function.  This
// is passed pointers to the elements of the array (which are
// void* pointers in our case because it's a Vector).
static int CompareCaseValue(const void* case1, const void* case2) {
  CaseLabelASTNode* node1 = *(CaseLabelASTNode**)case1;
  CaseLabelASTNode* node2 = *(CaseLabelASTNode**)case2;

  // NOTE that we don't care what the actual value returned is, as long
  // as its sign is correct.
  return (int)(node1->value - node2->value);
}

typedef struct {
  SwitchStatementASTNode* switch_node;
  int switch_level;
} SwitchResolver;

static void CheckJumpBypassedDeclarations(ASTNode* target, ASTNode* jump,
                                          GotoStatementASTNode* g,
                                          const char* jump_kind,
                                          const char* target_kind,
                                          bool diagnose);

// Check for duplicate case labels and defaults in statement.
// Also fill in the 'cases' vector in the switch statement AST node
// and the default_node if present.
static void ResolveSwitchStatement(ASTNode* node, void* data, int child_id,
                                   VisitorMode mode) {
  SwitchResolver* resolver = data;
  SwitchStatementASTNode* switch_node = resolver->switch_node;
  
  // We can't descend into nested switch statements.  If we are looking
  // at a switch, change the switch_level in the finder.
  if (node->op == AST_OP(switch)) {
    if (mode == kVisitPreChildren) {
      resolver->switch_level++;
    } else if (mode == kVisitPostChildren) {
      resolver->switch_level--;
    }
  }
  
  // If we are in a nested switch statememnt stop here.
  if (resolver->switch_level > 1) {
    return;
  }
  
  if (mode == kVisitPreChildren && node->op == AST_OP(case)) {
     CaseLabelASTNode* case_node = (CaseLabelASTNode*)node;

     if (case_node->expr == NULL) {
       // This is a default node.
       if (switch_node->default_node != NULL) {
         SemanticError(node, "Duplicate default in switch statement");
       }
       switch_node->default_node = case_node;
       return;
     }

     // Convert the case expression to the type of the switch controlling
     // expression.
     NormalConversion(case_node->expr, switch_node->expr->type);

     // Case labels need to be constant integer expressions.
     if (!EvaluateIntegerExpression(case_node->expr, &case_node->value)) {
       SemanticError(switch_node->expr,
                     "Case labels must be constant integral expressions");
     }
    // Calculate min, max and density.
     if (case_node->value < switch_node->min_case_value) {
       switch_node->min_case_value = case_node->value;
     }
     if (case_node->value > switch_node->max_case_value) {
       switch_node->max_case_value = case_node->value;
     }
     VectorAppend(&switch_node->cases, node);
   }
}

// Look at all the cases for the switch and determine what integer type
// we should use for the control expression.
static Type DetermineControlType(SwitchStatementASTNode* node) {
  size_t num_cases = node->cases.length;

  // Calculate the max bit width for all case constants.  This can be used
  // by the backend to optimize comparisons.
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    if (case_value < 0) {
      case_value = -case_value;
    }
    if (case_value == 0) {
      continue;
    }
    int width = 8;
    if (case_value < (1LL << 8)) {
      width = 1;
    } else if (case_value < (1LL << 16)) {
      width = 2;
    } else if (case_value < (1LL << 32)) {
      width = 4;
    }
    if (width > node->max_case_width) {
      node->max_case_width = width;
    }
  }
  
  // Determine int type to which to convert control expression.  This is based
  // on the max width of the cases in the statement.
  Type control_type = kTypeInt;
  switch (node->max_case_width) {
    case 1:
      control_type = kTypeChar;
      break;
    case 2:
      if (compiler->int_size == 2) {
        control_type = kTypeInt;
      } else {
        control_type = kTypeShort;
      }
      break;
    case 4:
      if (compiler->long_size == 4) {
        control_type = kTypeLong;
      } else {
        control_type = kTypeInt;
      }
      break;
    case 8:
      if (compiler->long_size == 8) {
        control_type = kTypeLong;
      } else {
        control_type = kTypeLongLong;
      }
      break;

  }
  return control_type;
}


static void AnalyzeEnumSwitch(SwitchStatementASTNode* node, Type control_type) {
  BitSet enum_constants = {0};
  BitSet found_constants = {0};
  Enum* info = node->expr->type->info.enum_info;
  assert(info != NULL);
  for (size_t i = 0; i < info->constants.length; i++) {
    Symbol* ec = info->constants.value.p[i];
    BitSetInsert(&enum_constants, ec->value.ivalue);
  }
  
  // Go through all the cases and make sure they are in the enum_constants.
  size_t num_cases = node->cases.length;
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
     if (BitSetContains(&enum_constants, case_value)) {
       BitSetInsert(&found_constants, case_value);
     } else {
       SemanticWarning(&node->base, "switch",
                       "Case value %" PRId64 " is not valid for enumeration %s",
                       case_value, info->tag_name->value);
     }
  }
  
  // Now check that we have included all the constants as cases.
  Vector missing_constants = {0};
  for (size_t i = 0; i < info->constants.length; i++) {
    Symbol* ec = info->constants.value.p[i];
    if (!BitSetContains(&found_constants,  ec->value.ivalue)) {
      VectorAppend(&missing_constants, ec);
    }
  }
  if (missing_constants.length > 0) {
    const char* warning = node->default_node == NULL ? "switch" : "switch-enum";
    if (missing_constants.length > 4) {
      Symbol* ec = missing_constants.value.p[0];
      SemanticWarning(&node->base, warning,
                      "Enum constant %s and %" PRId64 " others are not present in switch statement",
                      ec->name.value, missing_constants.length - 1);

    } else {
      for (size_t i = 0; i < missing_constants.length; i++) {
        Symbol* ec = missing_constants.value.p[i];
        SemanticWarning(&node->base, warning,
                        "Enum constant %s is not present in switch statement",
                        ec->name.value);
      }
    }
  } else {
    if (node->default_node == NULL) {
      // All cases covered.
      node->all_cases_covered = true;
    }
  }

  // Convert expr to int.  The conversion is to signed or unsigned
  if (node->all_cases_positive) {
    SemanticConvertType(node->expr,
                        NewTypeRecordWithSize(control_type | kTypeUnsigned, kQualPlain), kConvertNormal);
  } else {
    SemanticConvertType(node->expr,
                        NewTypeRecordWithSize(control_type, kQualPlain), kConvertNormal);
  }
  
  VectorDestruct(&missing_constants);
  BitSetDestruct(&enum_constants);
  BitSetDestruct(&found_constants);
}
  
static void AnalyzeSwitchStatement(SwitchStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->expr);

  if (!TypeIsIntegral(node->expr->type)) {
    SemanticError(node->expr, "Switch statements need an integer type");
    return;
  }
  
  SwitchResolver resolver = {
    .switch_node = node,
    .switch_level = 0,
  };
    
  // Visit the switch statement and all its children, collecting
  // case and defaults.
  ASTNodeVisit(&node->base, ResolveSwitchStatement, 0, &resolver);
  if (node->default_node == NULL) {
    SemanticWarning(&node->base, "switch-default",
                    "switch statement has no default label");
  } else {
    CheckJumpBypassedDeclarations((ASTNode*)node->default_node, (ASTNode*)node,
                                  NULL, "Switch statement", "Default label",
                                  true);
  }
  for (size_t i = 0; i < node->cases.length; i++) {
    CheckJumpBypassedDeclarations(node->cases.value.p[i], (ASTNode*)node, NULL,
                                  "Switch statement", "Case label", true);
  }

  size_t num_cases = node->cases.length;
  
  bool negative_cases = false;
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    if (case_value < 0) {
      negative_cases = true;
      break;
    }
  }
  node->all_cases_positive = !negative_cases;
  
  // Determine int type to which to convert control expression.  This is based
  // on the max width of the cases in the statement.
  Type control_type = DetermineControlType(node);
 
  // If not an enum convert to int.  We want to handle the conversion for
  // an enum differently.  If it has only positive cases we can convert to
  // unsigned int as that makes for better code generation (no sign extension).
  // This conversion is done in AnalyzeEnumSwitch.
  if (!TypeIsEnum(node->expr->type)) {
    // Convert expr to int.  The conversion is to signed or unsigned.
    if (TypeIsUnsigned(node->expr->type)) {
      SemanticConvertType(node->expr,
                          NewTypeRecordWithSize(control_type | kTypeUnsigned, kQualPlain), kConvertNormal);
    } else {
      SemanticConvertType(node->expr,
                          NewTypeRecordWithSize(control_type, kQualPlain), kConvertNormal);
    }
  }
  
  
  // Get an idea of the case density.
  // Density is mass/volume.  Let's say that the number of cases is the
  // masss and the distance between the min and max values is the volume.
  float mass = (node->cases.length + (node->default_node != NULL ? 1 : 0));
  float volume = node->max_case_value - node->min_case_value;
  if (volume != 0) {
    node->density = mass / volume;
  }

  // Now we sort the cases into ascending order.  We do this because it's better
  // for code generation.  We can do a branch table or a binary search if the
  // values are sorted.
  qsort(node->cases.value.p, node->cases.length, sizeof(void*), CompareCaseValue);

  // Check the case labels for duplicates. Since they are sorted we only need
  // to check for two adjacent values being the same.  This is faster than doing
  // an n^2 search for each value;
  for (size_t i = 0; i < num_cases - 1; i++) {
    int64_t case_value1 = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    int64_t case_value2 =
        ((CaseLabelASTNode*)(node->cases.value.p[i + 1]))->value;
    if (case_value1 == case_value2) {
      const char* filename;
      int lineno;
      int start, end;
      DecodeSourceLocation(
          ((CaseLabelASTNode*)node->cases.value.p[i])->expr->location, &filename,
          &lineno, &start, &end);
      SemanticError(((CaseLabelASTNode*)node->cases.value.p[i + 1])->expr,
                    "Duplicate case value %d; previous is at %s:%d",
                    case_value2, filename, lineno);
    }
  }
  
  // If the switch is on enumerated type we need to validate that the
  // cases are part of the enumeration and set the all_cases_covered
  // flag if we have all the valid enumeration constants.
  if (TypeIsEnum(node->expr->type)) {
    AnalyzeEnumSwitch(node, control_type);
  }
}

static void AnalyzeForStatement(ForStatementASTNode* node) {
  if (node->c1 != NULL) {
    if (node->c1->op == AST_OP(decl_list)) {
      // First "expression" might be a list of variable declaration statements.
      AnalyzeStatement(node->c1);
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node->c1;
      for (size_t i = 0; i < decls->declarations->length; i++) {
        VariableDeclarationASTNode* vardecl =
            (VariableDeclarationASTNode*)decls->declarations->value.p[i];
        if (!StorageIs(vardecl->symbol->storage, STO(auto)) &&
            !StorageIs(vardecl->symbol->storage, STO(register)) &&
            vardecl->symbol->storage != STO(implicit)) {
          SemanticError((ASTNode*)vardecl,
                        "Only auto or register variables "
                        "allowed in a for statement "
                        "declaration");
        }
      }
    } else {
      node->c1 = AnalyzeExpression(node->c1);
    }
  }

  node->c2 = AnalyzeExpression(node->c2);
  if (node->c2 != NULL) {
    SemanticConvertType(node->c2, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
    SemanticCheckScalarType(node->c2);
  }

  // Optional expression 3.
  node->c3 = AnalyzeExpression(node->c3);

  // Finally the statment.
  AnalyzeStatement(node->stmt);
}

static void AnalyzeCompoundStatement(CompoundStatementASTNode* node) {
  for (size_t i = 0; i < node->statements->length; i++) {
    AnalyzeStatement((ASTNode*)node->statements->value.p[i]);
  }
}

// Replace the CombinedStatementASTNode (a return statement) with a
// CompoundStatementASTNode containing arg assignemnts and a goto statement
// to a new label at the start of the function body,
static void AnalyzeTailRecursion(CombinedStatementASTNode* node, VectorASTNode* call) {
  static char tail_label_name[32];   // Unique name for label
  static int tail_label_num = 0;
  CompoundStatementASTNode* function_body =
        (CompoundStatementASTNode*)compiler->current_function->info.function.body;
  if (compiler->current_function->info.function.varargs ||
      compiler->current_function->info.function.unknown_args) {
    // Don't know the arguments, no tail recursion possible.
    return;
  }
  SourceLocation location = call->base.location;
  
  // Insert label as first statement in function body.
  snprintf(tail_label_name, sizeof(tail_label_name), "__tail_label_%d",
          tail_label_num++);
  LabelASTNode* label = (LabelASTNode*)NewLabelASTNode(tail_label_name,
                                                       NULL,
                                                       false,
                                                       location);
  CompoundASTNodeInsertStatement(function_body, (ASTNode*)label, 0);
   
  Vector* statements = NewVector();
  
  // Create assignment statements for all arguments to their formal args via
  // temporaries.
  size_t num_actual_args = call->children->length;
  TypeRecord* subtype = call->left->type;
  
  // Declare temporaries for all arguments and assign them from the call's
  // actual arguments.
  Vector* decls = NewVector();
  for (size_t i = 0; i < num_actual_args; i++) {
    ASTNode* actual = ASTNodeMove((ASTNode*)call->children->value.p[i]);
    Symbol* formal = (Symbol*)subtype->info.function.prototype.value.p[i];
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, formal->type);
  
    ASTNode* assignment = NewBinaryASTNode(AST_OP(assign),
                                            formal->type,
                                            location,
                                            NewIdentifierASTNode(temp,
                                                                 location),
                                            actual);
    ASTNode* decl = NewVariableDeclarationASTNode(temp, assignment, location);
    VectorAppend(decls, decl);
  }
  VectorAppend(statements, NewDeclarationListASTNode(decls, location));
  
  // Now assign all temporaries back to the formals.
  for (size_t i = 0; i < num_actual_args; i++) {
    VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)decls->value.p[i];
    ASTNode* temp = NewIdentifierASTNode(decl->symbol, location);
    Symbol* formal = (Symbol*)subtype->info.function.prototype.value.p[i];
    ASTNode* assignment = NewBinaryASTNode(AST_OP(assign),
                                           formal->type,
                                           location,
                                           NewIdentifierASTNode(formal,
                                                                location),
                                           temp);
    VectorAppend(statements, NewExpressionStatementASTNode(assignment,
                                                         location));
  }
  
  // Create goto statement to the label.
  ASTNode* goto_stmt = NewGotoStatementASTNode(NewString(label->name.value),
                                                   location);
  VectorAppend(statements, goto_stmt);
  ASTNode* result = NewCompoundStatementASTNode(statements, location);
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, result, true);
  AnalyzeStatement(result);
}

static bool TypeEqualIgnoringTopLevelQualifiers(TypeRecord* left,
                                                TypeRecord* right) {
  TypeRecord* left_copy = TypeRecordCopy(left);
  TypeRecord* right_copy = TypeRecordCopy(right);
  left_copy->qualifiers &= ~(kQualConst | kQualVolatile);
  right_copy->qualifiers &= ~(kQualConst | kQualVolatile);
  bool equal = TypeEqual(left_copy, right_copy);
  TypeRecordDelete(left_copy);
  TypeRecordDelete(right_copy);
  return equal;
}

static void SetCurrentFunctionReturnType(TypeRecord* deduced) {
  TypeRecord* old_return = compiler->current_function->next;
  deduced->qualifiers &= ~(kQualConst | kQualVolatile);
  TypeRecordIncRef(deduced);
  compiler->current_function->next = deduced;
  compiler->current_function->info.function.is_auto_return_deduced = true;
  TypeRecordDelete(old_return);
  if (compiler->current_function->info.function.symbol != NULL) {
    compiler->current_function->info.function.symbol->type =
        compiler->current_function;
  }
}

// Declared in type_internal.h; forward-declared here to avoid pulling the whole
// type-parser internal header into the semantic layer.
TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue);

static bool DeduceCurrentFunctionAutoReturn(ASTNode* return_value,
                                            ASTNode* diagnostic_node) {
  TypeRecord* pattern = compiler->current_function->next;
  // `decltype(auto)` return: the deduced type is decltype(return-expression),
  // which preserves the expression's value category (lvalue -> T&,
  // xvalue -> T&&, prvalue -> T) rather than decaying like plain `auto`.
  if (pattern != NULL && (pattern->type & kTypeDecltypeAuto) != 0 &&
      pattern->declarator == kDeclPrimitive) {
    TypeRecord* deduced;
    if (return_value == NULL) {
      deduced = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    } else if (return_value->value_category == kValueCategoryLvalue) {
      deduced = NewDecltypeReference(return_value->type, false);
    } else if (return_value->value_category == kValueCategoryXvalue) {
      deduced = NewDecltypeReference(return_value->type, true);
    } else {
      deduced = TypeRecordCopy(return_value->type);
    }
    SetCurrentFunctionReturnType(deduced);
    return true;
  }
  TypeRecord* initializer_type =
      return_value != NULL ? return_value->type
                           : NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* deduced = TypeDeduceAuto(pattern, initializer_type);
  if (return_value == NULL) {
    TypeRecordDelete(initializer_type);
  }
  if (deduced == NULL) {
    SemanticError(diagnostic_node, "Cannot deduce auto function return type");
    return false;
  }
  SetCurrentFunctionReturnType(deduced);
  return true;
}

static bool IsEligibleCXXReturnElisionValue(ASTNode* return_value) {
  if (!CompilerIsCXX() || return_value == NULL ||
      !TypeIsStructOrUnion(compiler->current_function->next) ||
      !TypeEqual(return_value->type, compiler->current_function->next)) {
    return false;
  }
  if (return_value->op == AST_OP(call)) {
    return return_value->value_category == kValueCategoryPrvalue;
  }
  if (return_value->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* sym = ((IdentifierASTNode*)return_value)->symbol;
  return sym != NULL && sym->flags.is_local && !sym->flags.is_argument &&
         !sym->flags.is_temp && !StorageIs(sym->storage, STO(static));
}

static bool IsEligibleCXXImplicitMoveReturnValue(ASTNode* return_value) {
  if (!CompilerIsCXX() || return_value == NULL ||
      return_value->op != AST_OP(identifier) ||
      compiler->current_function == NULL ||
      TypeIsReference(compiler->current_function->next) ||
      !TypeIsStructOrUnion(compiler->current_function->next) ||
      !TypeEqual(return_value->type, compiler->current_function->next)) {
    return false;
  }
  Symbol* sym = ((IdentifierASTNode*)return_value)->symbol;
  return sym != NULL && (sym->flags.is_local || sym->flags.is_argument) &&
         !sym->flags.is_temp && !StorageIs(sym->storage, STO(static));
}

static bool IsCXXFunctionArgumentReturnValue(ASTNode* return_value) {
  return return_value != NULL && return_value->op == AST_OP(identifier) &&
         ((IdentifierASTNode*)return_value)->symbol != NULL &&
         ((IdentifierASTNode*)return_value)->symbol->flags.is_argument;
}

static ASTNode* MaterializeCXXReturnByMove(ASTNode* return_value) {
  if (!CompilerIsCXX() || return_value == NULL ||
      compiler->current_function == NULL ||
      !TypeIsStructOrUnion(compiler->current_function->next) ||
      compiler->current_function->next->info.struct_info == NULL ||
      compiler->current_function->next->info.struct_info->tag_name == NULL) {
    return return_value;
  }

  TypeRecord* return_type = compiler->current_function->next;
  String* constructor_name = return_type->info.struct_info->tag_name;
  StructMember* constructor =
      FindStructMember(return_type->info.struct_info, constructor_name);
  if (constructor == NULL || !constructor->is_member_function ||
      constructor->symbol == NULL || constructor->symbol->type == NULL ||
      !constructor->symbol->type->info.function.is_constructor) {
    return return_value;
  }
  SourceLocation location = return_value->location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, return_type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(constructor_name->value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(return_value));
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), return_type, location, constructor_call,
                       result);
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  return analyzed;
}

static void AnalyzeReturnStatement(CombinedStatementASTNode* node) {
  if (compiler->current_function != NULL &&
      compiler->current_function->info.function.is_coroutine &&
      (((ASTNode*)node)->flags & kASTCoroutineLoweredReturn) == 0) {
    SemanticError((ASTNode*)node,
                  "return statement is not allowed in a coroutine; use co_return");
    return;
  }
  ASTNode* return_value = node->cond;
  if (return_value != NULL && return_value->op == AST_OP(asm)) {
    // Extension: return asm("foo") is allowed
    // Set the type of the asm statement to the return type of this function.
    ASTNodeSetType(return_value, compiler->current_function->next);
    return;
  }
  // `return {...};` returns a braced-init-list, which is not itself an
  // expression.  When the function's return type is already known (not an
  // `auto` return to be deduced), lower it to a temporary of that type so it is
  // analyzed and generated exactly like `return T{...};`.
  if (return_value != NULL && return_value->op == AST_OP(braced_init) &&
      compiler->current_function != NULL &&
      !TypeIsVoid(compiler->current_function->next) &&
      !TypeFunctionReturnContainsAuto(compiler->current_function)) {
    TypeRecord* return_type = compiler->current_function->next;
    TypeRecord* braced_target =
        TypeIsReference(return_type) ? return_type->next : return_type;
    ASTNode* lowered = LowerCXXBracedInitToTarget(return_value, braced_target);
    if (lowered != return_value) {
      ASTNodeReplaceChild((ASTNode*)node, 0, lowered, false);
      node->cond = lowered;
      return_value = lowered;
    }
  }
  return_value = AnalyzeExpression(return_value);
  if (return_value != node->cond) {
    ASTNodeReplaceChild((ASTNode*)node, 0, return_value, false);
  } else {
    node->cond = return_value;
  }

  if (TypeFunctionReturnContainsAuto(compiler->current_function)) {
    if (!DeduceCurrentFunctionAutoReturn(return_value, (ASTNode*)node)) {
      return;
    }
  } else if (return_value != NULL &&
             compiler->current_function->info.function.is_auto_return_deduced &&
             !TypeEqualIgnoringTopLevelQualifiers(
                 return_value->type, compiler->current_function->next)) {
    SemanticError(return_value, "Inconsistent auto function return type");
    return;
  }

  if (IsEligibleCXXImplicitMoveReturnValue(return_value)) {
    ASTValueCategory original_category = return_value->value_category;
    return_value->value_category = kValueCategoryXvalue;
    if (IsCXXFunctionArgumentReturnValue(return_value)) {
      ASTNode* materialized = MaterializeCXXReturnByMove(return_value);
      if (materialized != return_value) {
        ASTNodeReplaceChild((ASTNode*)node, 0, materialized, false);
        return_value = materialized;
      } else {
        return_value->value_category = original_category;
      }
    }
  }

  bool cxx_return_elision = IsEligibleCXXReturnElisionValue(return_value);
  if (!cxx_return_elision && return_value != NULL &&
      TypeIsStructOrUnion(compiler->current_function->next) &&
      TypeEqual(return_value->type, compiler->current_function->next) &&
      return_value->value_category != kValueCategoryPrvalue) {
    ASTNode* materialized = MaterializeCXXReturnByMove(return_value);
    if (materialized != return_value) {
      ASTNodeReplaceChild((ASTNode*)node, 0, materialized, false);
      return_value = materialized;
    }
  }

  // Check current function return type.
  if (TypeIsVoid(compiler->current_function->next)) {
    // C does not allow a return statement with a value in a void function.
    // Unless the value being returned is also void (from a function call)
    if (return_value != NULL) {
      if (!TypeIsVoid(return_value->type)) {
        SemanticError(return_value, "Cannot return a value from a void function");
      }
    }
  } else {
    // Function returns a value.
    if (return_value == NULL) {
      SemanticError((ASTNode*)node,
                    "Must return a value from a non-void function");
    } else if (TypeIsReference(compiler->current_function->next)) {
      TypeRecord* reference_type = compiler->current_function->next;
      NormalConversion(return_value, reference_type->next);
      if (reference_type->declarator == kDeclRValueReference) {
        if (return_value->value_category == kValueCategoryLvalue) {
          SemanticError(return_value,
                        "Rvalue reference return value must not be an lvalue");
        }
      } else if (return_value->value_category != kValueCategoryLvalue) {
        SemanticError(return_value, "Reference return value must be an lvalue");
      }
      return_value->flags |= kASTNeedAddress;
    } else if (!cxx_return_elision) {
      NormalConversion(return_value, compiler->current_function->next);
    }
  }
  
  // Returns a struct.  If this is a call node it might be subject
  // to RVO.
  if (return_value != NULL && (cxx_return_elision || compiler->optimize)) {
    if (TypeIsStructOrUnion(compiler->current_function->next)) {
      if (return_value->op == AST_OP(call)) {
        return_value->flags |= kASTRvoCall;
      } else if (return_value->op == AST_OP(identifier)) {
        // Named RVO places the returned variable directly in the caller's
        // return slot and elides the struct copy.  This is only valid for a
        // local automatic variable -- the backend can allocate such a variable
        // in the return slot.  A static/global/extern variable has its own
        // fixed storage, and a function argument is passed elsewhere, so those
        // must be copied into the return slot explicitly.  Mirror the
        // localvar test in NewIRVariable.
        Symbol* sym = ((IdentifierASTNode*)return_value)->symbol;
        if (sym != NULL && sym->flags.is_local && !sym->flags.is_argument &&
            !sym->flags.is_temp && !StorageIs(sym->storage, STO(static))) {
          return_value->flags |= kASTNrvoMarker;
          sym->is_nrvo = true;
        }
      }
    }
  }
  
  // Check for tail recursion.  This is a direct call to the current
  // function.
  if (OptLevel2() && return_value != NULL &&
      return_value->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)return_value;
    if (call->left->op == AST_OP(identifier)) {
      IdentifierASTNode* id_node = (IdentifierASTNode*)call->left;
      if (id_node->symbol == compiler->current_function->info.function.symbol) {
        AnalyzeTailRecursion(node, call);
      }
    }
  }
}

static void AnalyzeCoReturnStatement(CombinedStatementASTNode* node) {
  ASTNode* return_value = node->cond;
  if (return_value != NULL) {
    node->cond = AnalyzeExpression(return_value);
  }
  if (compiler->current_function == NULL ||
      !compiler->current_function->info.function.is_coroutine) {
    SemanticError((ASTNode*)node, "co_return used outside a coroutine");
  }
}

static void AnalyzeCaseLabel(CaseLabelASTNode* node) {
  if (node->expr != NULL) {
    // A case with no expression is used for 'default'.
    node->expr = AnalyzeExpression(node->expr);
  }
  if (node->stmt != NULL) {
    AnalyzeStatement(node->stmt);
  }
  // Since we don't know the type of the switch controlling expressions here
  // we delay the analysis of the case label statements to the analysis of the
  // switch statement.
}

static ASTNode* InitializerExpression(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  return initializer;
}

static ASTNode* NewSemanticInitExpression(Symbol* sym, ASTNode* initializer,
                                          SourceLocation location) {
  ASTNode* id = NewIdentifierASTNode(sym, location);
  id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewBinaryASTNode(AST_OP(init), TypeRecordCopy(sym->type), location, id,
                          initializer);
}

static TypeRecord* NewAutoReferenceType(bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, NewTypeRecord(kTypeAuto, kQualPlain));
  TypeRecordCalculateSize(ref);
  return ref;
}

static Symbol* FindStdSymbolByName(const char* name) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String symbol_name;
  StringInit(&symbol_name, name);
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &symbol_name);
  StringDestruct(&symbol_name);
  if (result.status != kInlineLookupUnique) {
    return NULL;
  }
  return result.symbol;
}

static TemplateArgument* NewNonTypeTemplateArgument(size_t value,
                                                    SourceLocation location) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = (long long)value;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = location;
  return arg;
}

static bool StructuredBindingTupleSize(TypeRecord* type, size_t* element_count) {
  Symbol* tuple_size = FindStdSymbolByName("tuple_size");
  if (tuple_size == NULL || !tuple_size->flags.is_template ||
      tuple_size->type == NULL || !TypeIsStructOrUnion(tuple_size->type)) {
    return false;
  }
  TypeRecord* object_type = TypeIsReference(type) ? type->next : type;
  Vector* args = NewVector();
  VectorAppend(args, NewTypeTemplateArgument(object_type));
  TypeRecord* tuple_size_type =
      TypeInstantiateClassTemplate(&compiler->syntax, tuple_size, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (tuple_size_type == NULL || !TypeIsStructOrUnion(tuple_size_type) ||
      tuple_size_type->info.struct_info == NULL) {
    TypeRecordDelete(tuple_size_type);
    return false;
  }
  StructMember* value = FindStructMemberByName(tuple_size_type->info.struct_info,
                                               "value");
  bool ok = value != NULL && value->symbol != NULL &&
            value->symbol->flags.value_set && value->symbol->value.ivalue >= 0;
  if (ok) {
    *element_count = (size_t)value->symbol->value.ivalue;
  }
  TypeRecordDelete(tuple_size_type);
  return ok;
}

static ASTNode* NewStructuredBindingGetCall(Symbol* hidden, size_t index,
                                            SourceLocation location) {
  Symbol* get = FindStdSymbolByName("get");
  if (get == NULL) {
    return NULL;
  }
  ASTNode* callee = NewIdentifierASTNode(get, location);
  Vector* template_arguments = NewVector();
  VectorAppend(template_arguments, NewNonTypeTemplateArgument(index, location));
  ((IdentifierASTNode*)callee)->template_arguments = template_arguments;
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewIdentifierASTNode(hidden, location));
  return NewVectorASTNode(AST_OP(call), NULL, location, callee, actuals);
}

static bool StructuredBindingDataMembers(TypeRecord* type, Vector* members,
                                         ASTNode* diagnostic_node) {
  TypeRecord* object_type = TypeIsReference(type) ? type->next : type;
  if (!TypeIsStructOrUnion(object_type) || object_type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = object_type->info.struct_info;
  if (str->is_union) {
    SemanticError(diagnostic_node,
                  "Cannot decompose union type in structured binding");
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (member->access != kAccessPublic) {
      SemanticError(diagnostic_node,
                    "Cannot decompose non-public member in structured binding");
      return false;
    }
    VectorAppend(members, member);
  }
  return true;
}

static ASTNode* NewStructuredBindingElementAccess(Symbol* hidden,
                                                  TypeRecord* hidden_type,
                                                  size_t index,
                                                  StructMember* member,
                                                  SourceLocation location) {
  ASTNode* object = NewIdentifierASTNode(hidden, location);
  TypeRecord* object_type = TypeIsReference(hidden_type) ? hidden_type->next
                                                        : hidden_type;
  if (TypeIsFixedArray(object_type)) {
    return NewBinaryASTNode(
        AST_OP(subscript), NULL, location, object,
        NewIntConstantASTNode((int64_t)index,
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
  }
  ASTNode* member_access = NewBinaryASTNode(
      AST_OP(dot), NULL, location, object,
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location));
  return member_access;
}

static bool LowerStructuredBindingDeclaration(DeclarationListASTNode* list,
                                              size_t index) {
  StructuredBindingASTNode* binding =
      (StructuredBindingASTNode*)list->declarations->value.p[index];
  SourceLocation location = binding->base.location;
  Symbol* hidden =
      SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(binding->declared_type));
  hidden->flags.is_local = true;
  hidden->flags.is_defined = true;
  hidden->location = location;

  ASTNode* initializer = binding->initializer;
  binding->initializer = NULL;
  ASTNode* hidden_decl = NewVariableDeclarationASTNode(
      hidden, NewSemanticInitExpression(hidden, initializer, location),
      location);
  list->declarations->value.p[index] = hidden_decl;

  AnalyzeStatement(hidden_decl);

  TypeRecord* object_type = TypeIsReference(hidden->type) ? hidden->type->next
                                                         : hidden->type;
  Vector elements;
  VectorInit(&elements);
  bool array_binding = TypeIsFixedArray(object_type);
  bool tuple_like_binding = false;
  size_t element_count = 0;
  if (array_binding) {
    element_count = (size_t)object_type->info.array.size.fixed;
  } else if (StructuredBindingTupleSize(hidden->type, &element_count)) {
    tuple_like_binding = true;
  } else if (StructuredBindingDataMembers(hidden->type, &elements,
                                          hidden_decl)) {
    element_count = elements.length;
  } else {
    VectorDestruct(&elements);
    SemanticError(hidden_decl, "Cannot decompose type in structured binding");
    ASTNodeDelete((ASTNode*)binding);
    return false;
  }

  if (element_count != binding->symbols->length) {
    SemanticError(hidden_decl,
                  "Structured binding declaration has wrong number of names");
    VectorDestruct(&elements);
    ASTNodeDelete((ASTNode*)binding);
    return false;
  }

  for (size_t i = 0; i < binding->symbols->length; i++) {
    Symbol* sym = binding->symbols->value.p[i];
    SymbolSetType(sym, NewAutoReferenceType(false));
    ASTNode* access =
        tuple_like_binding
            ? NewStructuredBindingGetCall(hidden, i, location)
            : NewStructuredBindingElementAccess(
                  hidden, hidden->type, i,
                  array_binding ? NULL : elements.value.p[i], location);
    if (access == NULL) {
      SemanticError(hidden_decl, "Cannot find get for tuple-like structured binding");
      VectorDestruct(&elements);
      ASTNodeDelete((ASTNode*)binding);
      return false;
    }
    ASTNode* decl = NewVariableDeclarationASTNode(
        sym, NewSemanticInitExpression(
                 sym, NewExpressionInitializerASTNode(access, location),
                 location),
        location);
    AnalyzeStatement(decl);
    VectorInsertAfter(list->declarations, index + i, decl);
  }
  VectorDestruct(&elements);
  ASTNodeDelete((ASTNode*)binding);
  return true;
}

void AnalyzeVariableDeclaration(VariableDeclarationASTNode* node) {
  node->initializer = AnalyzeExpression(node->initializer);
  bool is_cxx_local_static =
      CompilerIsCXX() && node->symbol != NULL &&
      StorageIs(node->symbol->storage, STO(static));
  if (is_cxx_local_static &&
      node->local_static_init_kind == kLocalStaticInitUnclassified) {
    if (node->initializer == NULL) {
      node->local_static_init_kind = kLocalStaticInitConstant;
    } else if (node->initializer->op == AST_OP(init)) {
      BinaryASTNode* init = (BinaryASTNode*)node->initializer;
      bool constant = InitializerIsLinkTimeConstant(init->right);
      node->local_static_init_kind =
          constant ? kLocalStaticInitConstant : kLocalStaticInitDynamic;
      if (constant) {
        node->initializer->flags |= kASTStaticInit;
      }
    } else {
      ASTNode* candidate = InitializerExpression(node->initializer);
      Symbol* constant_callee = NULL;
      if (candidate != NULL && candidate->op == AST_OP(call)) {
        VectorASTNode* call = (VectorASTNode*)candidate;
        if (call->left != NULL && call->left->op == AST_OP(identifier)) {
          constant_callee = ((IdentifierASTNode*)call->left)->symbol;
        }
      }
      ASTNode* constant_init =
          constant_callee != NULL && constant_callee->flags.is_constexpr
              ? ConstexprObjectInitializerForSymbol(
                    node->symbol, node->initializer->location)
              : NULL;
      if (constant_init != NULL) {
        ASTNode* simplified =
            AnalyzeInitializer(node->symbol->type, constant_init, true);
        ASTNodeDelete(node->initializer);
        node->initializer = NewSemanticInitExpression(
            node->symbol, simplified, node->base.location);
        node->initializer->flags |= kASTStaticInit;
        node->local_static_init_kind = kLocalStaticInitConstant;
      } else {
        node->local_static_init_kind = kLocalStaticInitDynamic;
        if (node->symbol->flags.is_constexpr ||
            node->symbol->flags.is_constinit) {
          SemanticError(
              node->initializer,
              node->symbol->flags.is_constinit
                  ? "constinit variable initializer is not a constant expression"
                  : "constexpr variable initializer is not a constant expression");
        }
      }
    }
  }
  ASTNode* initializer_expr = InitializerExpression(node->initializer);
  bool constructor_call = false;
  if (initializer_expr != NULL && initializer_expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)initializer_expr;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
      constructor_call =
          callee != NULL && TypeIsFunction(callee->type) &&
          callee->type->info.function.is_constructor;
    }
  }
  bool side_effect_initializer =
      node->initializer != NULL && node->initializer->op == AST_OP(stmt_expr);
  bool cxx_return_elision_initializer =
      CompilerIsCXX() && initializer_expr != NULL &&
      initializer_expr->op == AST_OP(call) &&
      TypeIsStructOrUnion(node->symbol->type) &&
      TypeEqual(initializer_expr->type, node->symbol->type);
  if (node->initializer != NULL && !constructor_call &&
      !side_effect_initializer && !cxx_return_elision_initializer) {
    NormalConversion(node->initializer, node->symbol->type);
  }
  if (!TypeIsReference(node->symbol->type)) {
    node->initializer =
        AppendCXXFullExpressionTemporaryDestructors(node->initializer);
  }
}

void AnalyzeDeclarationList(DeclarationListASTNode* node) {
  for (size_t i = 0; i < node->declarations->length; i++) {
    ASTNode* decl = node->declarations->value.p[i];
    if (decl != NULL && decl->op == AST_OP(structured_binding)) {
      LowerStructuredBindingDeclaration(node, i);
      continue;
    }
    AnalyzeStatement(decl);
  }
}

static void FindLabel(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  GotoStatementASTNode* goto_node = data;
  if (mode != kVisitPreChildren) {
    return;
  }
  if (goto_node->label != NULL) {
    // Already found, nothing to do.
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(goto_node->label_name, &label->name)) {
      goto_node->label = node;
    }
  }
}

static bool DeclarationHasAutomaticStorage(VariableDeclarationASTNode* decl) {
  if (decl == NULL || decl->symbol == NULL) {
    return false;
  }
  return StorageIs(decl->symbol->storage, STO(auto)) ||
         StorageIs(decl->symbol->storage, STO(register)) ||
         decl->symbol->storage == STO(implicit);
}

static bool CXXDeclarationHasBypassedInitialization(
    VariableDeclarationASTNode* decl) {
  return CompilerIsCXX() && DeclarationHasAutomaticStorage(decl) &&
         decl->initializer != NULL;
}

static bool DeclarationCannotBeBypassed(VariableDeclarationASTNode* decl) {
  return TypeIsVLA(decl->base.type) ||
         CXXDeclarationHasBypassedInitialization(decl);
}

static void GetBypassedDeclarations(DeclarationListASTNode* decl_list,
                                    Vector* bypassed_decls) {
  for (size_t i = 0; i < decl_list->declarations->length; i++) {
    VariableDeclarationASTNode* decl = decl_list->declarations->value.p[i];
    if (DeclarationCannotBeBypassed(decl)) {
      VectorAppend(bypassed_decls, decl);
    }
  }
}

static void CollectBypassedDeclarationsInCompound(
    CompoundStatementASTNode* compound, size_t begin, size_t end,
    Vector* bypassed_decls) {
  if (end > compound->statements->length) {
    end = compound->statements->length;
  }
  for (size_t i = begin; i < end; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (stmt->op == AST_OP(decl_list)) {
      GetBypassedDeclarations((DeclarationListASTNode*)stmt, bypassed_decls);
    }
  }
}

static void CollectBypassedDeclarationsOnTargetPath(Vector* target_path,
                                                    int start_index,
                                                    Vector* bypassed_decls) {
  for (int i = start_index; i > 0; i--) {
    ASTNode* block = target_path->value.p[i];
    ASTNode* branch = target_path->value.p[i - 1];
    if (block->op == AST_OP(compound) && branch->child_id > 0) {
      CollectBypassedDeclarationsInCompound((CompoundStatementASTNode*)block, 0,
                                            (size_t)branch->child_id,
                                            bypassed_decls);
    }
  }
}

static void ReportJumpError(ASTNode* jump, ASTNode* target,
                            Vector* bypassed_decls,
                            const char* jump_kind,
                            const char* target_kind) {
  SemanticError(jump, "%s cannot enter a scope by bypassing a variable "
                "initialization",
                jump_kind);
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(target->location, &filename, &lineno, &start, &end);
  ReportNote(filename, lineno, "%s is here", target_kind);

  for (size_t i = 0; i < bypassed_decls->length; i++) {
    VariableDeclarationASTNode* decl = bypassed_decls->value.p[i];
    DecodeSourceLocation(decl->base.location, &filename, &lineno, &start, &end);
    if (TypeIsVLA(decl->base.type)) {
      ReportNote(filename, lineno,
                 "Variable length array '%s' is bypassed",
                 decl->symbol->name.value);
    } else {
      ReportNote(filename, lineno, "Initialization of '%s' is bypassed",
                 decl->symbol->name.value);
    }
  }
}

// Check a control transfer to a label-like target. It is invalid to enter a
// scope by bypassing C/C++ VLA definitions or non-vacuous C++ initialization.
static void CheckJumpBypassedDeclarations(ASTNode* target, ASTNode* jump,
                                          GotoStatementASTNode* g,
                                          const char* jump_kind,
                                          const char* target_kind,
                                          bool diagnose) {
  Vector target_path = {0};
  Vector jump_path = {0};
  ASTNode* node = target;
  while (node != NULL) {
    VectorAppend(&target_path, node);
    node = node->parent;
  }
  node = jump;
  while (node != NULL) {
    VectorAppend(&jump_path, node);
    node = node->parent;
  }

  ASTNode* lca = NULL;
  int target_index = (int)target_path.length - 1;
  int jump_index = (int)jump_path.length - 1;
  while (target_index >= 0 && jump_index >= 0) {
    if (target_path.value.p[target_index] != jump_path.value.p[jump_index]) {
      lca = target_path.value.p[target_index + 1];
      break;
    }
    target_index--;
    jump_index--;
  }
  if (lca == NULL) {
    if (jump_index < 0) {
      lca = jump_path.value.p[0];
    } else if (target_index < 0) {
      lca = target_path.value.p[0];
    }
  }
  assert(lca != NULL);
  if (g != NULL) {
    g->lca = lca;
  }

  ASTNode* jump_branch = jump_index >= 0 ? jump_path.value.p[jump_index] : NULL;
  ASTNode* target_branch =
      target_index >= 0 ? target_path.value.p[target_index] : NULL;

  Vector bypassed_decls = {0};
  if (diagnose) {
    if (lca->op == AST_OP(compound) && jump_branch != NULL &&
        target_branch != NULL &&
        jump_branch->child_id < target_branch->child_id) {
      CollectBypassedDeclarationsInCompound(
          (CompoundStatementASTNode*)lca, (size_t)jump_branch->child_id + 1,
          (size_t)target_branch->child_id, &bypassed_decls);
    }

    int target_path_start =
        lca->op == AST_OP(compound) ? target_index : target_index + 1;
    CollectBypassedDeclarationsOnTargetPath(&target_path, target_path_start,
                                            &bypassed_decls);

    if (bypassed_decls.length != 0) {
      ReportJumpError(jump, target, &bypassed_decls, jump_kind, target_kind);
    }
  }

  VectorDestruct(&bypassed_decls);
  VectorDestruct(&target_path);
  VectorDestruct(&jump_path);
}

static void CheckGoto(ASTNode* label, ASTNode* jump, GotoStatementASTNode* g) {
  bool diagnose = (jump->flags & kASTCompilerGeneratedGoto) == 0;
  CheckJumpBypassedDeclarations(label, jump, g, "Goto", "Label", diagnose);
}


void AnalyzeGotoStatement(GotoStatementASTNode* node) {
  // Look for label matching the goto.
  // If we find it, set the 'label' field of the node to point to it.  This
  // does not own the label node.
  // In C, labels are global to the whole function.
  ASTNodeVisit(compiler->current_function->info.function.body, FindLabel, 0, node);

  if (node->label == NULL) {
    SemanticError((ASTNode*)node, "Undefined label '%s'", node->label_name->value);
  } else {
    node->label->flags |= kASTLabelUsed;
    CheckGoto(node->label, &node->base, node);
  }
}


typedef struct {
  String* label_name;
  bool found;
} DuplicateLabelFinder;

static void FindDuplicateLabel(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  DuplicateLabelFinder* finder = data;

  if (mode != kVisitPreChildren) {
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(finder->label_name, &label->name)) {
      if (finder->found) {
        SemanticError(&label->base, "Duplicate label '%s'",
                      finder->label_name->value);
      }
      finder->found = true;
    }
  }
}

void AnalyzeLabel(LabelASTNode* node) {
  DuplicateLabelFinder finder = {
    .label_name = &node->name,
    .found = false,
  };
  
  ASTNodeVisit(compiler->current_function->info.function.body,
               FindDuplicateLabel, 0, &finder);
  if (node->stmt != NULL) {
    AnalyzeStatement(node->stmt);
  }
}

static void AnalyzeCatchStatement(CatchASTNode* node) {
  if (node->is_catch_all) {
    if (node->symbol != NULL) {
      SemanticError((ASTNode*)node, "catch (...) cannot declare a variable");
    }
  } else if (node->symbol == NULL || node->symbol->type == NULL) {
    SemanticError((ASTNode*)node, "catch handler requires a declaration");
  } else if (TypeIsVoid(node->symbol->type) ||
             TypeIsFunction(node->symbol->type)) {
    SemanticError((ASTNode*)node, "Invalid catch declaration type");
  }
  SemanticEnterCatchHandler();
  AnalyzeStatement(node->stmt);
  SemanticLeaveCatchHandler();
}

static void AnalyzeTryStatement(TryASTNode* node) {
  if (!CompilerExceptionsEnabled()) {
    SemanticError((ASTNode*)node,
                  "cannot use 'try' with exception handling disabled "
                  "(-fno-exceptions)");
  }
  AnalyzeStatement(node->try_stmt);
  bool seen_catch_all = false;
  for (size_t i = 0; i < node->catches->length; i++) {
    CatchASTNode* catch_node = node->catches->value.p[i];
    if (seen_catch_all) {
      SemanticError((ASTNode*)catch_node,
                    "catch (...) must be the last catch handler");
    }
    AnalyzeCatchStatement(catch_node);
    if (catch_node->is_catch_all) {
      seen_catch_all = true;
    }
  }
}

static void FindUnusedLabel(ASTNode* node, void* data, int child_id,
                            VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node->op != AST_OP(label)) {
    return;
  }
  LabelASTNode* label = (LabelASTNode*)node;
  if (!label->named && (node->flags & kASTLabelUsed) == 0) {
    SemanticWarning(node, "unused-label", "label '%s' defined but not used",
                    label->name.value);
  }
}

void CheckUnusedLabels(ASTNode* body) {
  ASTNodeVisit(body, FindUnusedLabel, 0, NULL);
}

void AnalyzeStatement(ASTNode* node) {
  if (node == NULL || (node->flags & kASTAnalyzed) != 0) {
    return;
  }

  switch (node->op) {
    case AST_OP(decl_list):
      AnalyzeDeclarationList((DeclarationListASTNode*)node);
      break;
    case AST_OP(vardecl):
      AnalyzeVariableDeclaration((VariableDeclarationASTNode*)node);
      break;
    case AST_OP(expr):
      AnalyzeExpressionStatement((ExpressionStatementASTNode*)node);
      break;
    case AST_OP(static_assert):
      AnalyzeStaticAssert((StaticAssertASTNode*)node);
      break;
    case AST_OP(compound):
      AnalyzeCompoundStatement((CompoundStatementASTNode*)node);
      break;
    case AST_OP(if):
      AnalyzeIfStatement((IfStatementASTNode*)node);
      break;
    case AST_OP(while):
      AnalyzeWhileStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(do):
      AnalyzeDoStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(switch):
      AnalyzeSwitchStatement((SwitchStatementASTNode*)node);
      break;
    case AST_OP(for):
      AnalyzeForStatement((ForStatementASTNode*)node);
      break;
    case AST_OP(return ):
      AnalyzeReturnStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(co_return):
      AnalyzeCoReturnStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(case):
      AnalyzeCaseLabel((CaseLabelASTNode*)node);
      break;
    case AST_OP(goto):
      AnalyzeGotoStatement((GotoStatementASTNode*)node);
      break;
    case AST_OP(label):
      AnalyzeLabel((LabelASTNode*)node);
      break;
    case AST_OP(try):
      AnalyzeTryStatement((TryASTNode*)node);
      break;
    case AST_OP(catch):
      AnalyzeCatchStatement((CatchASTNode*)node);
      break;

    case AST_OP(break):
    case AST_OP(continue):
      // No analysis needed for these.
      break;
    case AST_OP(asm):
      AnalyzeAsmStatement((AsmASTNode*)node);
      break;

    default:
      assert(false);
  }
  node->flags |= kASTAnalyzed;
}
