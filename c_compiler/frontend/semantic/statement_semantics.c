//
//  statement_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_semantics.h"
#include "contracts.h"
#include "expansion_semantics.h"
#include "reflection_semantics.h"
#include <assert.h>
#include <stdio.h>
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
    case AST_OP(builtin_prefetch):
    case AST_OP(builtin_start_lifetime):
    case AST_OP(builtin_observable_checkpoint):
    case AST_OP(builtin_trap):
    case AST_OP(builtin_unreachable):
      return true;
    case AST_OP(builtin_expect): {
      VectorASTNode* builtin = (VectorASTNode*)node;
      for (size_t i = 0; i < builtin->children->length; i++) {
        if (ExpressionHasSideEffects(builtin->children->value.p[i])) {
          return true;
        }
      }
      return false;
    }
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
  if (expr->op == AST_OP(compound_literal)) {
    ASTNode* sym = ((CompoundLiteralASTNode*)expr)->sym;
    return sym != NULL && sym->op == AST_OP(identifier)
               ? ((IdentifierASTNode*)sym)->symbol
               : NULL;
  }
  if (expr->op == AST_OP(init)) {
    return CXXTemporaryConstructionResultSymbol(((BinaryASTNode*)expr)->right);
  }
  if (expr->op == AST_OP(designated_init)) {
    return CXXTemporaryConstructionResultSymbol(
        ((DesignatedInitializerASTNode*)expr)->init);
  }
  ASTNode* single = CXXSingleExpressionInitializer(expr);
  if (single != NULL && single != expr) {
    return CXXTemporaryConstructionResultSymbol(single);
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

static bool CXXInitializerConstructsInPlace(ASTNode* init);

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
      if (CXXTemporaryConstructionResultSymbol(expr) == sym &&
          CXXSameClassIgnoringQualifiers(parent->type, sym->type) &&
          CXXInitializerConstructsInPlace(designated->init)) {
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
  if (init->op == AST_OP(cast)) {
    return CXXInitializerConstructsInPlace(((CastASTNode*)init)->expr);
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
  Vector parameter_temps;
  // Temporaries the expression already destroys itself.  An inlined call brings
  // the callee's body along, cleanups and all, so a temporary made inside the
  // callee is seen here as if it belonged to the caller's full expression --
  // and destroying it a second time, unconditionally, runs a destructor on
  // storage that the caller's path may never have constructed.
  Vector already_destroyed;
} CXXTemporaryCollection;

static bool CXXTemporaryIsFunctionParameterObject(ASTNode* node, Symbol* sym) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTFunctionParameterTemporary) != 0 &&
        CXXTemporaryConstructionResultSymbol(current) == sym) {
      return true;
    }
  }
  return false;
}

static bool CXXNodeIsWithinTemporaryCleanup(ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTTemporaryCleanupCall) != 0) {
      return true;
    }
  }
  return false;
}

// Record the receiver of every destructor call the expression already makes, so
// CollectCXXTemporarySymbols can leave those temporaries alone.
static void CollectCXXDestroyedTemporarySymbols(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier) ||
      !CXXNodeIsWithinTemporaryCleanup(node)) {
    return;
  }
  CXXTemporaryCollection* collection = data;
  Symbol* sym = ((IdentifierASTNode*)node)->symbol;
  if (sym != NULL &&
      !VectorContainsPointer(&collection->already_destroyed, sym)) {
    VectorAppend(&collection->already_destroyed, sym);
  }
}

static void CollectCXXTemporarySymbols(ASTNode* node, void* data, int child_id,
                                       VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      CXXNodeIsWithinTemporaryCleanup(node)) {
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
  if (CXXTemporaryNeedsDestructor(sym)) {
    if (VectorContainsPointer(&collection->already_destroyed, sym)) {
      return;
    }
    if (CXXTemporaryIsFunctionParameterObject(node, sym)) {
      if (!VectorContainsPointer(&collection->parameter_temps, sym)) {
        VectorAppend(&collection->parameter_temps, sym);
      }
    } else if (!VectorContainsPointer(&collection->temps, sym)) {
      VectorAppend(&collection->temps, sym);
    }
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
  call = AnalyzeExpression(call);
  call->flags |= kASTTemporaryCleanupCall;
  return call;
}

ASTNode* AppendCXXFullExpressionTemporaryDestructors(ASTNode* expr) {
  if (!CompilerIsCXX() || expr == NULL ||
      (compiler->current_function != NULL &&
       (compiler->current_function->info.function.is_coroutine ||
        compiler->current_function->info.function.coroutine_frame_type != NULL))) {
    return expr;
  }
  CXXTemporaryCollection collection;
  VectorInit(&collection.temps);
  VectorInit(&collection.elided);
  VectorInit(&collection.parameter_temps);
  VectorInit(&collection.already_destroyed);
  ASTNodeVisit(expr, CollectCXXDestroyedTemporarySymbols, 0, &collection);
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
  VectorDestruct(&collection.parameter_temps);
  VectorDestruct(&collection.already_destroyed);
  return expr;
}

static ASTNode* AppendCXXFullExpressionTemporaryDestructorsPreservingValue(
    ASTNode* expr) {
  if (!CompilerIsCXX() || expr == NULL ||
      (compiler->current_function != NULL &&
       (compiler->current_function->info.function.is_coroutine ||
        compiler->current_function->info.function.coroutine_frame_type != NULL))) {
    return expr;
  }
  CXXTemporaryCollection collection;
  VectorInit(&collection.temps);
  VectorInit(&collection.elided);
  VectorInit(&collection.parameter_temps);
  VectorInit(&collection.already_destroyed);
  ASTNodeVisit(expr, CollectCXXDestroyedTemporarySymbols, 0, &collection);
  ASTNodeVisit(expr, CollectCXXTemporarySymbols, 0, &collection);
  if (collection.temps.length == 0) {
    VectorDestruct(&collection.temps);
    VectorDestruct(&collection.elided);
    VectorDestruct(&collection.parameter_temps);
    VectorDestruct(&collection.already_destroyed);
    return expr;
  }

  Symbol* saved = SyntaxNewTemporary(&compiler->syntax, expr->type);
  ASTNode* assignment = NewBinaryASTNode(
      AST_OP(assign), NULL, expr->location,
      NewIdentifierASTNode(saved, expr->location), expr);
  ASTNode* sequence = AnalyzeExpression(assignment);
  for (size_t i = collection.temps.length; i > 0; i--) {
    Symbol* sym = collection.temps.value.p[i - 1];
    ASTNode* destructor =
        NewCXXTemporaryDestructorCall(sym, expr->location);
    if (destructor != NULL) {
      sequence = AnalyzeExpression(NewBinaryASTNode(
          AST_OP(comma), NULL, expr->location, sequence, destructor));
    }
  }
  ASTNode* result =
      AnalyzeExpression(NewIdentifierASTNode(saved, expr->location));
  sequence = AnalyzeExpression(NewBinaryASTNode(
      AST_OP(comma), NULL, expr->location, sequence, result));
  VectorDestruct(&collection.temps);
  VectorDestruct(&collection.elided);
  VectorDestruct(&collection.parameter_temps);
  VectorDestruct(&collection.already_destroyed);
  return sequence;
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

static ASTNode* CXXRangeForDeclarationInitializer(ASTNode* range_decl) {
  if (range_decl == NULL || range_decl->op != AST_OP(decl_list)) {
    return NULL;
  }
  DeclarationListASTNode* declarations =
      (DeclarationListASTNode*)range_decl;
  if (declarations->declarations == NULL ||
      declarations->declarations->length != 1) {
    return NULL;
  }
  ASTNode* declaration = declarations->declarations->value.p[0];
  if (declaration == NULL || declaration->op != AST_OP(vardecl)) {
    return NULL;
  }
  return ((VariableDeclarationASTNode*)declaration)->initializer;
}

static bool CXXRangeForTemporaryIsExtended(
    CXXTemporaryCollection* collection, Symbol* symbol, Symbol* direct) {
  if (VectorContainsPointer(&collection->parameter_temps, symbol)) {
    return false;
  }
  return CompilerCXXAtLeast(kLanguageStandardCXX23) || symbol == direct;
}

void CXXCollectRangeForInitializerTemporaries(ASTNode* range_decl,
                                              Vector* out) {
  ASTNode* initializer = CXXRangeForDeclarationInitializer(range_decl);
  if (initializer == NULL || out == NULL) {
    return;
  }
  CXXTemporaryCollection collection;
  VectorInit(&collection.temps);
  VectorInit(&collection.elided);
  VectorInit(&collection.parameter_temps);
  VectorInit(&collection.already_destroyed);
  ASTNodeVisit(initializer, CollectCXXDestroyedTemporarySymbols, 0,
               &collection);
  ASTNodeVisit(initializer, CollectCXXTemporarySymbols, 0, &collection);
  Symbol* direct = CXXTemporaryConstructionResultSymbol(initializer);
  for (size_t i = collection.temps.length; i > 0; i--) {
    Symbol* symbol = collection.temps.value.p[i - 1];
    if (CXXRangeForTemporaryIsExtended(&collection, symbol, direct)) {
      VectorAppend(out, symbol);
    }
  }
  VectorDestruct(&collection.temps);
  VectorDestruct(&collection.elided);
  VectorDestruct(&collection.parameter_temps);
  VectorDestruct(&collection.already_destroyed);
}

static ASTNode* AppendRangeForEndOfInitializerTemporaryDestructors(
    ASTNode* initializer) {
  CXXTemporaryCollection collection;
  VectorInit(&collection.temps);
  VectorInit(&collection.elided);
  VectorInit(&collection.parameter_temps);
  VectorInit(&collection.already_destroyed);
  ASTNodeVisit(initializer, CollectCXXDestroyedTemporarySymbols, 0,
               &collection);
  ASTNodeVisit(initializer, CollectCXXTemporarySymbols, 0, &collection);
  Symbol* direct = CXXTemporaryConstructionResultSymbol(initializer);
  for (size_t i = collection.temps.length; i > 0; i--) {
    Symbol* symbol = collection.temps.value.p[i - 1];
    if (CXXRangeForTemporaryIsExtended(&collection, symbol, direct)) {
      continue;
    }
    ASTNode* destructor =
        NewCXXTemporaryDestructorCall(symbol, initializer->location);
    if (destructor != NULL) {
      initializer = NewBinaryASTNode(AST_OP(comma), destructor->type,
                                     initializer->location, initializer,
                                     destructor);
      initializer->flags |= kASTAnalyzed;
    }
  }
  // Function-parameter temporaries are never lifetime-extended by a range-for
  // initializer.  They are tracked separately from ordinary temporaries so
  // they are not also destroyed by the generic full-expression cleanup path;
  // destroy them explicitly at the end of this initializer.
  for (size_t i = collection.parameter_temps.length; i > 0; i--) {
    Symbol* symbol = collection.parameter_temps.value.p[i - 1];
    ASTNode* destructor =
        NewCXXTemporaryDestructorCall(symbol, initializer->location);
    if (destructor != NULL) {
      initializer = NewBinaryASTNode(AST_OP(comma), destructor->type,
                                     initializer->location, initializer,
                                     destructor);
      initializer->flags |= kASTAnalyzed;
    }
  }
  VectorDestruct(&collection.temps);
  VectorDestruct(&collection.elided);
  VectorDestruct(&collection.parameter_temps);
  VectorDestruct(&collection.already_destroyed);
  return initializer;
}

void AppendRangeForTemporaryDestructorStatements(ASTNode* range_decl,
                                                        Vector* out) {
  Vector temporaries;
  VectorInit(&temporaries);
  CXXCollectRangeForInitializerTemporaries(range_decl, &temporaries);
  for (size_t i = 0; i < temporaries.length; i++) {
    AppendLocalDestructorStatements(temporaries.value.p[i], out);
  }
  VectorDestruct(&temporaries);
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
    if ((compound->base.flags & kASTRangeForInitializer) != 0 && i == 1) {
      AppendRangeForTemporaryDestructorStatements(stmt, out);
    }
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
    if (p->op == AST_OP(for) || p->op == AST_OP(expansion_for) ||
        p->op == AST_OP(while) ||
        p->op == AST_OP(do) || p->op == AST_OP(switch)) {
      return p;
    }
  }
  return NULL;
}

static ASTNode* CXXEnclosingLoop(ASTNode* node) {
  for (ASTNode* p = node->parent; p != NULL; p = p->parent) {
    if (p->op == AST_OP(for) || p->op == AST_OP(expansion_for) ||
        p->op == AST_OP(while) ||
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

static ASTNode* ElideCXXMemberInitializerPrvalue(ASTNode* expr) {
  if (expr == NULL || expr->op != AST_OP(call) ||
      (expr->flags & kASTCXXMemberInitializer) == 0) {
    return expr;
  }
  VectorASTNode* outer = (VectorASTNode*)expr;
  if (outer->left == NULL || outer->left->op != AST_OP(identifier) ||
      outer->children == NULL || outer->children->length < 2) {
    return expr;
  }
  Symbol* outer_constructor = ((IdentifierASTNode*)outer->left)->symbol;
  ASTNode* actual =
      outer->children->value.p[outer->children->length - 1];
  if (outer_constructor == NULL || outer_constructor->type == NULL ||
      !TypeIsFunction(outer_constructor->type) ||
      !outer_constructor->type->info.function.is_constructor ||
      actual == NULL || actual->op != AST_OP(comma)) {
    return expr;
  }
  BinaryASTNode* materialized = (BinaryASTNode*)actual;
  ASTNode* inner_node = materialized->left;
  Symbol* temporary =
      CXXTemporaryConstructionResultSymbol(materialized->right);
  if (inner_node == NULL || inner_node->op != AST_OP(call) ||
      temporary == NULL) {
    return expr;
  }
  VectorASTNode* inner = (VectorASTNode*)inner_node;
  if (inner->left == NULL || inner->left->op != AST_OP(identifier) ||
      inner->children == NULL || inner->children->length == 0) {
    return expr;
  }
  Symbol* inner_constructor = ((IdentifierASTNode*)inner->left)->symbol;
  if (inner_constructor == NULL || inner_constructor->type == NULL ||
      !TypeIsFunction(inner_constructor->type) ||
      !inner_constructor->type->info.function.is_constructor ||
      outer_constructor->type->info.function.cxx_member_owner !=
          inner_constructor->type->info.function.cxx_member_owner ||
      outer_constructor->type->info.function.cxx_member_owner == NULL) {
    return expr;
  }

  ASTNode* target = outer->children->value.p[0];
  ASTNode* old_inner_target = inner->children->value.p[0];
  if (target == NULL || old_inner_target == NULL ||
      old_inner_target->op != AST_OP(address) ||
      ((UnaryASTNode*)old_inner_target)->sub == NULL ||
      ((UnaryASTNode*)old_inner_target)->sub->op != AST_OP(identifier) ||
      ((IdentifierASTNode*)((UnaryASTNode*)old_inner_target)->sub)->symbol !=
          temporary) {
    return expr;
  }
  VectorSet(outer->children, 0, NULL);
  VectorSet(inner->children, 0, target);
  target->parent = inner_node;
  target->child_id = 0;
  ASTNodeDelete(old_inner_target);
  materialized->left = NULL;
  inner_node->parent = expr->parent;
  inner_node->child_id = expr->child_id;
  ASTNodeDelete(expr);
  return inner_node;
}

static void AnalyzeExpressionStatement(ExpressionStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  node->expr = ElideCXXMemberInitializerPrvalue(node->expr);
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
  compiler->constant_evaluation_required_depth++;
  expr = AnalyzeExpression(expr);
  compiler->constant_evaluation_required_depth--;
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
    String message;
    StringInit(&message, node->message.value);
    if (node->message_expr == NULL ||
        SyntaxEvaluateStaticAssertMessage(node->message_expr, &message)) {
      SemanticError((ASTNode*)node, "%s", message.value);
    }
    StringDestruct(&message);
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
  if (node->is_constexpr) {
    compiler->constant_evaluation_required_depth++;
  }
  node->cond = AnalyzeExpression(node->cond);
  if (node->is_constexpr) {
    compiler->constant_evaluation_required_depth--;
  }
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
  node->cond =
      AppendCXXFullExpressionTemporaryDestructorsPreservingValue(node->cond);
  AnalyzeStatement(node->if_part);
  AnalyzeStatement(node->else_part);
  SemanticCheckScalarType(node->cond);
}

static bool IsTriviallyEmptyIterationBody(ASTNode* stmt) {
  return stmt == NULL ||
         (stmt->op == AST_OP(compound) &&
          (stmt->flags & kASTSourceEmptyCompound) != 0);
}

static bool CXXLoopConditionIsConstantTrue(ASTNode* cond) {
  int64_t value = 0;
  return cond != NULL && IsConstantExpression(cond) &&
         EvaluateIntegerExpression(cond, &value) && value != 0;
}

static void AnalyzeWhileStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticConvertType(node->cond, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  SemanticCheckScalarType(node->cond);
  bool trivial_infinite =
      CompilerCXXAtLeast(kLanguageStandardCXX11) &&
      IsTriviallyEmptyIterationBody(node->stmt) &&
      CXXLoopConditionIsConstantTrue(node->cond);
  node->cond =
      AppendCXXFullExpressionTemporaryDestructorsPreservingValue(node->cond);
  AnalyzeStatement(node->stmt);
  if (trivial_infinite) {
    node->base.flags |= kASTTrivialInfiniteLoop;
  }
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeDoStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticConvertType(node->cond, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  SemanticCheckScalarType(node->cond);
  bool trivial_infinite =
      CompilerCXXAtLeast(kLanguageStandardCXX11) &&
      IsTriviallyEmptyIterationBody(node->stmt) &&
      CXXLoopConditionIsConstantTrue(node->cond);
  node->cond =
      AppendCXXFullExpressionTemporaryDestructorsPreservingValue(node->cond);
  AnalyzeStatement(node->stmt);
  if (trivial_infinite) {
    node->base.flags |= kASTTrivialInfiniteLoop;
  }
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
  if (node->expr != NULL && TypeIsStructOrUnion(node->expr->type)) {
    SemanticConvertCXXSwitchCondition(node->expr);
  }
  node->expr =
      AppendCXXFullExpressionTemporaryDestructorsPreservingValue(node->expr);
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
  for (size_t i = 0; i + 1 < num_cases; i++) {
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

static void AnalyzeExpansionStatement(ExpansionStatementASTNode* node) {
  if (node->init_stmt != NULL) {
    AnalyzeStatement(node->init_stmt);
  }
  // The range is expanded at compile time, so what the expansion reads has to
  // stay interpretable: inlining a call within it leaves behind a body the
  // constant evaluator rejects.  The loop body analyzed further down is
  // ordinary run-time code and keeps its inlining.
  compiler->constant_evaluation_required_depth++;
  if (node->initializer != NULL) {
    node->initializer = AnalyzeExpression(node->initializer);
  }
  int errors_before_materialization = NumErrors();
  ASTNode* materialized =
      SemanticMaterializeExpansionStatement(node, NULL, NULL);
  compiler->constant_evaluation_required_depth--;
  if (materialized != (ASTNode*)node) {
    if (node->base.parent != NULL) {
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, materialized,
                          true);
    }
    AnalyzeStatement(materialized);
    return;
  }
  if (NumErrors() == errors_before_materialization) {
    AnalyzeStatement(node->stmt);
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
      node->c1 =
          AppendCXXFullExpressionTemporaryDestructors(node->c1);
    }
  }

  node->c2 = AnalyzeExpression(node->c2);
  bool constant_true_condition = node->c2 == NULL;
  if (node->c2 != NULL) {
    SemanticConvertType(node->c2, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
    SemanticCheckScalarType(node->c2);
    constant_true_condition = CXXLoopConditionIsConstantTrue(node->c2);
    node->c2 =
        AppendCXXFullExpressionTemporaryDestructorsPreservingValue(node->c2);
  }

  // Optional expression 3.
  node->c3 = AnalyzeExpression(node->c3);
  node->c3 = AppendCXXFullExpressionTemporaryDestructors(node->c3);

  // Finally the statment.
  AnalyzeStatement(node->stmt);
  if (CompilerCXXAtLeast(kLanguageStandardCXX11) &&
      node->c3 == NULL && IsTriviallyEmptyIterationBody(node->stmt) &&
      constant_true_condition) {
    node->base.flags |= kASTTrivialInfiniteLoop;
  }
}

static void AnalyzeCompoundStatement(CompoundStatementASTNode* node) {
  for (size_t i = 0; i < node->statements->length; i++) {
    ASTNode* statement = node->statements->value.p[i];
    AnalyzeStatement(statement);
    if (i == 0 && (node->base.flags & kASTRangeForInitializer) != 0 &&
        statement != NULL && statement->op == AST_OP(decl_list)) {
      DeclarationListASTNode* declarations =
          (DeclarationListASTNode*)statement;
      if (declarations->declarations != NULL &&
          declarations->declarations->length == 1) {
        ASTNode* declaration = declarations->declarations->value.p[0];
        if (declaration != NULL && declaration->op == AST_OP(vardecl)) {
          VariableDeclarationASTNode* variable =
              (VariableDeclarationASTNode*)declaration;
          variable->initializer =
              AppendRangeForEndOfInitializerTemporaryDestructors(
                  variable->initializer);
        }
      }
    }
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
  bool decltype_auto =
      old_return != NULL && old_return->declarator == kDeclPrimitive &&
      (old_return->type & kTypeDecltypeAuto) != 0;
  deduced->qualifiers &= ~(kQualConst | kQualVolatile);
  TypeRecordIncRef(deduced);
  compiler->current_function->next = deduced;
  compiler->current_function->info.function.is_auto_return_deduced = true;
  compiler->current_function->info.function.is_decltype_auto_return_deduced =
      decltype_auto;
  TypeRecordDelete(old_return);
  if (compiler->current_function->info.function.symbol != NULL) {
    compiler->current_function->info.function.symbol->type =
        compiler->current_function;
  }
}

/* A placeholder return type deduced from a type-dependent operand -- a lambda
 * inside a template whose body is analyzed against the still-symbolic enclosing
 * parameters -- would freeze a template parameter into the signature and make
 * the enclosing closure permanently dependent.  Deduction for such a body
 * belongs to the instantiation, where the operand is concrete, so leave the
 * placeholder in place. */
static bool DeducedReturnTypeIsStillDependent(TypeRecord* deduced) {
  return deduced != NULL &&
         (TypeIsUnknown(deduced) || TypeContainsTemplateParameter(deduced));
}

static bool DeduceCurrentFunctionAutoReturn(ASTNode* return_value,
                                            ASTNode* diagnostic_node) {
  TypeRecord* pattern = compiler->current_function->next;
  if (pattern != NULL && (pattern->type & kTypeDecltypeAuto) != 0 &&
      pattern->declarator == kDeclPrimitive) {
    TypeRecord* deduced = TypeDeduceDecltypeAuto(return_value);
    if (deduced == NULL) {
      SemanticError(diagnostic_node,
                    "Cannot deduce decltype(auto) function return type");
      return false;
    }
    if (DeducedReturnTypeIsStillDependent(deduced)) {
      TypeRecordDelete(deduced);
      return true;
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
  if (DeducedReturnTypeIsStillDependent(deduced)) {
    TypeRecordDelete(deduced);
    return true;
  }
  SetCurrentFunctionReturnType(deduced);
  return true;
}

static void CollectValueReturns(ASTNode* node, void* data, int child_id,
                                VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node != NULL && node->op == AST_OP(return) &&
      ((CombinedStatementASTNode*)node)->cond != NULL) {
    VectorAppend((Vector*)data, node);
  }
}

static void ResetStaleDependentReturnExpression(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if (node->type == NULL || TypeIsUnknown(node->type) ||
      (node->flags & kASTDependentFunctorCall) != 0) {
    node->flags &= ~(kASTAnalyzed | kASTDependentFunctorCall);
  }
}

void StatementFinishAutoReturnDeduction(TypeRecord* func,
                                        ASTNode* diagnostic_node) {
  if (func == NULL || !TypeIsFunction(func) ||
      !TypeFunctionReturnContainsAuto(func)) {
    return;
  }
  Vector returns;
  VectorInit(&returns);
  ASTNodeVisit(func->info.function.body, CollectValueReturns, 0, &returns);
  TypeRecord* saved_function = compiler->current_function;
  compiler->current_function = func;
  if (returns.length == 0) {
    // No `return` with an operand: deduce as if from `return;` at the closing
    // brace ([dcl.spec.auto]/8), which yields `void` for a plain `auto` or
    // `decltype(auto)` placeholder and is an error for anything else (`auto*`).
    DeduceCurrentFunctionAutoReturn(/*return_value=*/NULL, diagnostic_node);
  }
  // Otherwise the placeholder survived analysis of the returns themselves: the
  // body was cloned from an already-analyzed template pattern (so re-analysis
  // is a no-op) or every operand was still type-dependent at the time.  Retry
  // against the operands as they stand now; if they are still dependent the
  // deduction stays deferred to a later, more concrete instantiation.
  for (size_t i = 0;
       i < returns.length && TypeFunctionReturnContainsAuto(func); i++) {
    CombinedStatementASTNode* statement = returns.value.p[i];
    if (statement->cond == NULL) {
      continue;
    }
    // Instantiation rebuilds parts of a cloned body (a fold expansion, say)
    // without clearing the enclosing statement's analyzed flag, so the walk
    // above may not have reached the operand.  Type it here rather than leave
    // the signature unresolved for the call that asked.
    if ((statement->cond->flags & kASTAnalyzed) == 0 ||
        statement->cond->type == NULL ||
        TypeIsUnknown(statement->cond->type) ||
        (statement->cond->flags & kASTDependentFunctorCall) != 0) {
      ASTNodeVisit(statement->cond, ResetStaleDependentReturnExpression, 0,
                   NULL);
      ASTNode* analyzed = AnalyzeExpression(statement->cond);
      if (analyzed != statement->cond) {
        ASTNodeReplaceChild((ASTNode*)statement, 0, analyzed, false);
      }
      statement->cond = analyzed;
    }
    if (statement->cond != NULL && statement->cond->type != NULL) {
      DeduceCurrentFunctionAutoReturn(statement->cond, diagnostic_node);
    }
  }
  compiler->current_function = saved_function;
  VectorDestruct(&returns);
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

static bool IsCXXMoveEligibleIdentifier(ASTNode* expression) {
  if (!CompilerIsCXX() || expression == NULL ||
      expression->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* sym = ((IdentifierASTNode*)expression)->symbol;
  if (sym == NULL || (!sym->flags.is_local && !sym->flags.is_argument) ||
      sym->flags.is_temp || StorageIs(sym->storage, STO(static))) {
    return false;
  }
  TypeRecord* object_type = sym->type;
  if (object_type == NULL) {
    return false;
  }
  if (TypeIsReference(object_type)) {
    if (object_type->declarator != kDeclRValueReference) {
      return false;
    }
    object_type = object_type->next;
  }
  // DaveCC expression nodes do not retain top-level cv-qualification from an
  // identifier.  Keep const operands as lvalues so overload resolution cannot
  // incorrectly select a non-const move constructor.
  return object_type != NULL && !TypeIsConst(object_type) &&
         !TypeIsVolatile(object_type);
}

static bool IsEligibleCXXImplicitMoveReturnValue(ASTNode* return_value) {
  if (!IsCXXMoveEligibleIdentifier(return_value) ||
      compiler->current_function == NULL) {
    return false;
  }
  TypeRecord* return_type = compiler->current_function->next;
  TypeRecord* target_type = return_type;
  if (TypeIsReference(return_type)) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23) ||
        return_type->declarator != kDeclRValueReference) {
      return false;
    }
    target_type = return_type->next;
  } else if (!TypeIsStructOrUnion(return_type)) {
    return false;
  }
  return target_type != NULL &&
         TypeEqualIgnoringTopLevelQualifiers(return_value->type, target_type);
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

// [class.temporary] identifies the glvalues that continue to designate an
// object produced by temporary materialization.  DaveCC represents the
// materialized root as a compound literal and preserves the standard's
// propagating expression forms around it.
static bool CXXExpressionDesignatesTemporary(ASTNode* expr) {
  if (expr == NULL) {
    return false;
  }
  if (expr->value_category == kValueCategoryPrvalue ||
      expr->op == AST_OP(compound_literal) ||
      (expr->flags & kASTCXXBracedTemporary) != 0) {
    return true;
  }
  switch (expr->op) {
    case AST_OP(cast):
      return CXXExpressionDesignatesTemporary(((CastASTNode*)expr)->expr);
    case AST_OP(comma):
      return CXXExpressionDesignatesTemporary(
          ((BinaryASTNode*)expr)->right);
    case AST_OP(dot):
    case AST_OP(dotstar):
      return CXXExpressionDesignatesTemporary(
          ((BinaryASTNode*)expr)->left);
    case AST_OP(subscript): {
      ASTNode* array = ((BinaryASTNode*)expr)->left;
      return array != NULL && TypeIsArray(array->type) &&
             CXXExpressionDesignatesTemporary(array);
    }
    case AST_OP(question): {
      ASTNode* alternatives = ((BinaryASTNode*)expr)->right;
      if (alternatives == NULL || alternatives->op != AST_OP(colon)) {
        return false;
      }
      BinaryASTNode* colon = (BinaryASTNode*)alternatives;
      return CXXExpressionDesignatesTemporary(colon->left) ||
             CXXExpressionDesignatesTemporary(colon->right);
    }
    default:
      return false;
  }
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
             compiler->current_function->info.function.is_auto_return_deduced) {
    TypeRecord* candidate =
        compiler->current_function->info.function
                .is_decltype_auto_return_deduced
            ? TypeDeduceDecltypeAuto(return_value)
            : TypeRecordCopy(return_value->type);
    bool consistent =
        candidate != NULL &&
        TypeEqualIgnoringTopLevelQualifiers(
            candidate, compiler->current_function->next);
    TypeRecordDelete(candidate);
    if (!consistent) {
      SemanticError(return_value, "Inconsistent auto function return type");
      return;
    }
  }

  if (IsEligibleCXXImplicitMoveReturnValue(return_value)) {
    ASTValueCategory original_category = return_value->value_category;
    return_value->value_category = kValueCategoryXvalue;
    if (IsCXXFunctionArgumentReturnValue(return_value) &&
        !TypeIsReference(compiler->current_function->next)) {
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
  Symbol* named_return_symbol =
      return_value != NULL && return_value->op == AST_OP(identifier)
          ? ((IdentifierASTNode*)return_value)->symbol
          : NULL;
  TypeRecord* named_return_object_type =
      named_return_symbol != NULL ? named_return_symbol->type : NULL;
  if (named_return_object_type != NULL &&
      TypeIsReference(named_return_object_type)) {
    named_return_object_type = named_return_object_type->next;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX23) &&
      named_return_object_type != NULL &&
      TypeIsConst(named_return_object_type) &&
      TypeIsStructOrUnion(compiler->current_function->next) &&
      TypeEqualIgnoringTopLevelQualifiers(
          return_value->type, compiler->current_function->next)) {
    CXXValidateReturnInitialization(compiler->current_function->next,
                                    return_value);
  }
  if (!cxx_return_elision && return_value != NULL &&
      TypeIsStructOrUnion(compiler->current_function->next) &&
      TypeEqualIgnoringTopLevelQualifiers(
          return_value->type, compiler->current_function->next) &&
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
      // A conversion can replace the operand node, so classify and generate
      // from the converted expression retained by the return statement.
      return_value = node->cond;
      bool temporary_return =
          CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          CXXExpressionDesignatesTemporary(return_value);
      if (temporary_return) {
        SemanticError(
            return_value,
            "returned reference cannot be initialized with a temporary "
            "expression");
      } else if (reference_type->declarator == kDeclRValueReference) {
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
  
  // A conversion above can wrap the returned expression in another node, so
  // ask the statement what it returns rather than what it was handed.
  if (return_value != NULL) {
    return_value = node->cond;
  }

  // Returns a struct.  If this is a call node it might be subject
  // to RVO.
  if (return_value != NULL && (cxx_return_elision || compiler->optimize)) {
    if (TypeIsStructOrUnion(compiler->current_function->next)) {
      // Only a call that produces the returned object itself can be built in
      // the caller's return slot.  If its result still has to be converted,
      // what the caller gets back is the conversion's output, and the call has
      // to leave its own result somewhere the conversion can read it.
      if (return_value->op == AST_OP(call) &&
          TypeEqual(return_value->type, compiler->current_function->next)) {
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
      AST_OP(dot), NULL, location, object, NewStructMemberASTNode(member, location));
  ((StructMemberASTNode*)((BinaryASTNode*)member_access)->right)->byte_offset =
      member->byte_offset;
  return member_access;
}

static Symbol* CloneStructuredBindingSymbol(Symbol* source, const char* name) {
  TypeRecord* type =
      source != NULL && source->type != NULL
          ? TypeRecordCopy(source->type)
          : NewTypeRecord(kTypeAuto, kQualPlain);
  Symbol* replacement =
      NewSymbol(name, type, source != NULL ? source->storage : STO(implicit));
  if (source != NULL) {
    replacement->flags = source->flags;
    replacement->location = source->location;
    replacement->alignment = source->alignment;
    replacement->namespace_ = source->namespace_;
    replacement->value = source->value;
    replacement->structured_binding_pack_size =
        source->structured_binding_pack_size;
    AttributeListDestruct(&replacement->attributes);
    AttributeListClone(&replacement->attributes, &source->attributes);
  }
  replacement->flags.is_parameter_pack = false;
  replacement->structured_binding_pack_size = -2;
  return replacement;
}

static void AddStructuredBindingSymbolMapping(Map* map, Symbol* source,
                                              Symbol* replacement) {
  if (map == NULL || source == NULL || replacement == NULL) {
    return;
  }
  MapKeyValue kv;
  kv.key.p = source;
  kv.value.p = replacement;
  MapInsert(map, kv);
}

static bool ExpandClonedStructuredBindingSymbols(
    StructuredBindingASTNode* binding, size_t element_count, Map* symbol_map,
    Map* pack_symbol_map, ASTNode* diagnostic_node) {
  if (binding == NULL || symbol_map == NULL) {
    return true;
  }

  int pack_index = binding->pack_index;
  size_t logical_count = binding->symbols->length;
  if (binding->names->length != logical_count) {
    SemanticError(diagnostic_node,
                  "Invalid structured binding name and symbol count");
    return false;
  }
  if (pack_index < 0) {
    Vector* replacements = NewVector();
    for (size_t i = 0; i < logical_count; i++) {
      Symbol* source = binding->symbols->value.p[i];
      String* source_name =
          i < binding->names->length ? binding->names->value.p[i] : NULL;
      Symbol* replacement = CloneStructuredBindingSymbol(
          source, source_name != NULL ? source_name->value : "__binding");
      AddStructuredBindingSymbolMapping(symbol_map, source, replacement);
      VectorAppend(replacements, replacement);
    }
    VectorDelete(binding->symbols);
    binding->symbols = replacements;
    return true;
  }

  if ((size_t)pack_index >= logical_count || logical_count == 0) {
    SemanticError(diagnostic_node,
                  "Invalid structured binding pack position");
    return false;
  }
  size_t fixed_count = logical_count - 1;
  if (fixed_count > element_count) {
    SemanticError(
        diagnostic_node,
        "Structured binding declaration has more fixed names than elements");
    return false;
  }
  size_t pack_length = element_count - fixed_count;
  Vector* expanded_names = NewVector();
  Vector* expanded_symbols = NewVector();
  Vector* pack_elements = NewVector();

  for (size_t element = 0; element < element_count; element++) {
    bool is_pack_element =
        element >= (size_t)pack_index &&
        element < (size_t)pack_index + pack_length;
    size_t logical_index =
        is_pack_element
            ? (size_t)pack_index
            : (element < (size_t)pack_index
                   ? element
                   : element - pack_length + 1);
    Symbol* source = binding->symbols->value.p[logical_index];
    String* source_name = binding->names->value.p[logical_index];
    String generated_name = {0};
    const char* replacement_name = source_name->value;
    if (is_pack_element) {
      StringPrintf(&generated_name, "%s$pack%zu", source_name->value,
                   element - (size_t)pack_index);
      replacement_name = generated_name.value;
    }
    Symbol* replacement =
        CloneStructuredBindingSymbol(source, replacement_name);
    VectorAppend(expanded_names, NewString(replacement_name));
    VectorAppend(expanded_symbols, replacement);
    StringDestruct(&generated_name);
    if (is_pack_element) {
      VectorAppend(pack_elements, replacement);
    } else {
      AddStructuredBindingSymbolMapping(symbol_map, source, replacement);
    }
  }

  Symbol* source_pack = binding->symbols->value.p[(size_t)pack_index];
  if (pack_symbol_map != NULL) {
    MapKeyValue kv;
    kv.key.p = source_pack;
    kv.value.p = pack_elements;
    MapInsert(pack_symbol_map, kv);
  } else {
    VectorDelete(pack_elements);
  }
  VectorDestructWithContents(binding->names,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
  VectorDelete(binding->names);
  VectorDelete(binding->symbols);
  binding->names = expanded_names;
  binding->symbols = expanded_symbols;
  binding->pack_index = -1;
  return true;
}

static bool LowerStructuredBindingDeclaration(DeclarationListASTNode* list,
                                              size_t index, Map* symbol_map,
                                              Map* pack_symbol_map) {
  StructuredBindingASTNode* binding =
      (StructuredBindingASTNode*)list->declarations->value.p[index];
  SourceLocation location = binding->base.location;
  Symbol* hidden = binding->condition_symbol;
  TypeRecord* hidden_type = TypeRecordCopy(binding->declared_type);
  Symbol* first_binding =
      binding->symbols != NULL && binding->symbols->length != 0
          ? binding->symbols->value.p[0]
          : NULL;
  bool is_constexpr =
      first_binding != NULL && first_binding->flags.is_constexpr;
  bool is_constinit =
      first_binding != NULL && first_binding->flags.is_constinit;
  if (is_constexpr && !TypeIsReference(hidden_type)) {
    hidden_type->qualifiers |= kQualConst;
  }
  if (hidden != NULL && symbol_map != NULL) {
    Symbol* source_hidden = hidden;
    hidden = MapFindPointerKey(symbol_map, source_hidden);
    if (hidden == NULL) {
      hidden = CloneStructuredBindingSymbol(
          source_hidden, source_hidden->name.value);
      AddStructuredBindingSymbolMapping(symbol_map, source_hidden, hidden);
    }
    binding->condition_symbol = hidden;
  }
  if (hidden != NULL) {
    SymbolSetType(hidden, hidden_type);
    TypeRecordDelete(hidden_type);
  } else {
    hidden = SyntaxNewTemporary(&compiler->syntax, hidden_type);
  }
  hidden->flags.is_local = true;
  hidden->flags.is_defined = true;
  hidden->flags.is_constexpr = is_constexpr;
  hidden->flags.is_constinit = is_constinit;
  hidden->storage = binding->storage;
  hidden->location = location;

  ASTNode* initializer = binding->initializer;
  binding->initializer = NULL;
  ASTNode* hidden_decl = NewVariableDeclarationASTNode(
      hidden, NewSemanticInitExpression(hidden, initializer, location),
      location);
  list->declarations->value.p[index] = hidden_decl;

  AnalyzeStatement(hidden_decl);
  if (binding->condition_symbol != NULL) {
    hidden->structured_binding_pack_size = -2;
  }

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

  if (!ExpandClonedStructuredBindingSymbols(
          binding, element_count, symbol_map, pack_symbol_map, hidden_decl)) {
    VectorDestruct(&elements);
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
    Symbol* symbol = binding->symbols->value.p[i];
    if (symbol != NULL) {
      symbol->structured_binding_pack_size = -2;
    }
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
    if (is_constexpr || is_constinit) {
      access = AnalyzeExpression(access);
      TypeRecord* element_type = access != NULL ? access->type : NULL;
      if (element_type == NULL && array_binding) {
        element_type = object_type->next;
      } else if (element_type == NULL && !tuple_like_binding) {
        StructMember* element = elements.value.p[i];
        element_type =
            element != NULL && element->symbol != NULL
                ? element->symbol->type
                : NULL;
      }
      if (element_type != NULL) {
        TypeRecord* concrete_element = TypeRecordCopy(element_type);
        if (TypeIsConst(object_type)) {
          concrete_element->qualifiers |= kQualConst;
        }
        TypeRecord* reference = NewReferenceTypeRecord(kQualPlain, false);
        TypeRecordChain(reference, concrete_element);
        TypeRecordCalculateSize(reference);
        SymbolSetType(sym, reference);
      }
    }
    ASTNode* decl = NewVariableDeclarationASTNode(
        sym, NewSemanticInitExpression(
                 sym, NewExpressionInitializerASTNode(access, location),
                 location),
        location);
    if (is_constexpr || is_constinit) {
      sym->is_constexpr_representable = true;
      sym->constexpr_reference_scope = compiler->current_function;
      ASTNodeDelete(sym->constexpr_initializer);
      sym->constexpr_initializer =
          ASTNodeClone(access, StaticAssertIdentityClone, NULL, NULL);
      // Analyze the invented reference using the ordinary structured-binding
      // path. Its constexpr property belongs to the binding declaration, while
      // representability is tracked separately from scalar constant folding.
      sym->flags.is_constexpr = false;
      sym->flags.is_constinit = false;
    }
    AnalyzeStatement(decl);
    sym->flags.is_constexpr = is_constexpr;
    sym->flags.is_constinit = is_constinit;
    VectorInsertAfter(list->declarations, index + i, decl);
  }
  VectorDestruct(&elements);
  ASTNodeDelete((ASTNode*)binding);
  return true;
}

ASTNode* SemanticMaterializeClonedStructuredBinding(
    ASTNode* node, Map* symbol_map, Map* pack_symbol_map) {
  if (node == NULL || node->op != AST_OP(structured_binding)) {
    return node;
  }
  Vector* declarations = NewVector();
  VectorAppend(declarations, node);
  DeclarationListASTNode* list =
      (DeclarationListASTNode*)NewDeclarationListASTNode(
          declarations, node->location);
  if (LowerStructuredBindingDeclaration(list, 0, symbol_map,
                                        pack_symbol_map)) {
    list->base.flags |= kASTAnalyzed;
  }
  return (ASTNode*)list;
}

static bool DetermineKnownStructuredBindingPackSize(
    StructuredBindingASTNode* binding, size_t* pack_size) {
  if (binding == NULL || binding->pack_index < 0 ||
      binding->initializer == NULL || pack_size == NULL) {
    return false;
  }
  ASTNode* value = binding->initializer;
  if (value->op == AST_OP(expr_init)) {
    value = ((ExpressionInitializerASTNode*)value)->expr;
  }
  TypeRecord* object_type = value != NULL ? value->type : NULL;
  if (object_type == NULL || TypeIsUnknown(object_type) ||
      TypeContainsAuto(object_type) ||
      TypeContainsTemplateParameter(object_type)) {
    return false;
  }
  if (TypeIsReference(object_type)) {
    object_type = object_type->next;
  }

  size_t element_count = 0;
  Vector elements;
  VectorInit(&elements);
  if (TypeIsFixedArray(object_type)) {
    element_count = (size_t)object_type->info.array.size.fixed;
  } else if (StructuredBindingTupleSize(object_type, &element_count)) {
    // element_count was filled by the tuple protocol.
  } else if (StructuredBindingDataMembers(object_type, &elements,
                                          (ASTNode*)binding)) {
    element_count = elements.length;
  } else {
    VectorDestruct(&elements);
    return false;
  }
  VectorDestruct(&elements);

  size_t fixed_count =
      binding->symbols->length > 0 ? binding->symbols->length - 1 : 0;
  if (fixed_count > element_count) {
    SemanticError(
        (ASTNode*)binding,
        "Structured binding declaration has more fixed names than elements");
    return false;
  }
  *pack_size = element_count - fixed_count;
  return true;
}

void AnalyzeVariableDeclaration(VariableDeclarationASTNode* node) {
  if (node->symbol != NULL && node->symbol->type != NULL) {
    node->symbol->type =
        SemanticResolveDependentSpliceType(node->symbol->type, (ASTNode*)node);
  }
  if (node->symbol != NULL) {
    SemanticAttachAnnotationAttributes(&node->symbol->attributes, node->symbol);
  }
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
  if (node->symbol != NULL && node->symbol->type != NULL &&
      TypeIsConstevalOnly(node->symbol->type) &&
      !TypeContainsTemplateParameter(node->symbol->type)) {
    if (!node->symbol->flags.is_constexpr &&
        !node->symbol->flags.invented) {
      SemanticError((ASTNode*)node,
                    "A variable of consteval-only reflection type must be "
                    "declared constexpr");
    }
    if (node->symbol->flags.is_local && compiler->current_function != NULL &&
        TypeIsFunction(compiler->current_function) &&
        !compiler->current_function->info.function.is_consteval) {
      SemanticError((ASTNode*)node,
                    "A local variable of consteval-only reflection type is "
                    "only permitted in an immediate function");
    }
    ReflectionValue* reflection =
        TypeIsReflection(node->symbol->type)
            ? SemanticEvaluateReflection(initializer_expr)
            : SemanticReflectionValueFromExpression(initializer_expr);
    if (TypeIsReflection(node->symbol->type) && reflection != NULL) {
      node->symbol->value.other = reflection;
      node->symbol->flags.value_set = true;
    }
  }
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
    if (!TypeIsReflection(node->symbol->type)) {
      NormalConversion(node->initializer, node->symbol->type);
    }
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
      StructuredBindingASTNode* binding = (StructuredBindingASTNode*)decl;
      if (binding->initializer != NULL) {
        binding->initializer = AnalyzeExpression(binding->initializer);
        if (binding->initializer != NULL) {
          binding->initializer->parent = decl;
          binding->initializer->child_id = 0;
        }
      }
      bool dependent =
          binding->pack_index >= 0 ||
          ExpressionIsTemplateDependent(binding->initializer) ||
          (binding->initializer != NULL &&
           binding->initializer->type != NULL &&
           (TypeIsUnknown(binding->initializer->type) ||
            TypeContainsAuto(binding->initializer->type) ||
            TypeContainsTemplateParameter(binding->initializer->type)));
      if (binding->pack_index >= 0 &&
          (size_t)binding->pack_index < binding->symbols->length) {
        size_t pack_size = 0;
        if (DetermineKnownStructuredBindingPackSize(binding, &pack_size)) {
          Symbol* pack =
              binding->symbols->value.p[(size_t)binding->pack_index];
          if (pack != NULL) {
            pack->structured_binding_pack_size = (int)pack_size;
          }
        }
      }
      if (dependent) {
        decl->flags |= kASTAnalyzed;
        continue;
      }
      LowerStructuredBindingDeclaration(node, i, NULL, NULL);
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
    case AST_OP(contract_assert):
      SemanticAnalyzeContractAssert((ContractAssertASTNode*)node);
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
    case AST_OP(expansion_for):
      AnalyzeExpansionStatement((ExpansionStatementASTNode*)node);
      break;
    case AST_OP(consteval_block):
      SemanticAnalyzeConstevalBlock((ConstevalBlockASTNode*)node);
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

bool SemanticAnalyzeStructuredBindingDecomposition(
    TypeRecord* type, ASTNode* diagnostic,
    StructuredBindingDecomposition* out) {
  if (out == NULL) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  VectorInit(&out->members);
  TypeRecord* object_type = type;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  if (object_type != NULL && TypeIsFixedArray(object_type)) {
    out->array_like = true;
    out->element_count = (size_t)object_type->info.array.size.fixed;
    return true;
  }
  if (StructuredBindingTupleSize(type, &out->element_count)) {
    out->tuple_like = true;
    return out->element_count > 0;
  }
  if (StructuredBindingDataMembers(type, &out->members, diagnostic)) {
    out->element_count = out->members.length;
    return out->element_count > 0;
  }
  return false;
}

void SemanticStructuredBindingDecompositionDestruct(
    StructuredBindingDecomposition* decomposition) {
  if (decomposition == NULL) {
    return;
  }
  VectorDestruct(&decomposition->members);
}

ASTNode* SemanticStructuredBindingElementAccess(
    Symbol* hidden, TypeRecord* hidden_type, size_t index,
    StructMember* member, bool tuple_like, SourceLocation location) {
  if (tuple_like) {
    return NewStructuredBindingGetCall(hidden, index, location);
  }
  return NewStructuredBindingElementAccess(hidden, hidden_type, index, member,
                                           location);
}

Symbol* SemanticCloneExpansionIterationSymbol(Symbol* source) {
  const char* name =
      source != NULL && source->name.value != NULL ? source->name.value
                                                   : "__item";
  return CloneStructuredBindingSymbol(source, name);
}

static ASTNode* HiddenBindingCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

ASTNode* SemanticCreateHiddenReferenceBinding(ASTNode* init_expr,
                                              SourceLocation location,
                                              Symbol** hidden_out) {
  TypeRecord* init_type =
      init_expr != NULL ? init_expr->type : NULL;
  if (init_type == NULL) {
    return NULL;
  }
  Symbol* hidden = SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(init_type));
  if (init_expr->value_category != kValueCategoryPrvalue &&
      !TypeIsReference(hidden->type)) {
    TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(ref, hidden->type);
    TypeRecordDelete(hidden->type);
    hidden->type = ref;
    TypeRecordCalculateSize(hidden->type);
  }
  hidden->flags.is_local = true;
  hidden->flags.is_defined = true;
  hidden->location = location;
  ASTNode* init_clone =
      ASTNodeClone(init_expr, HiddenBindingCloneNode, NULL, NULL);
  ASTNode* hidden_decl = NewVariableDeclarationASTNode(
      hidden,
      NewSemanticInitExpression(
          hidden, NewExpressionInitializerASTNode(init_clone, location),
          location),
      location);
  if (hidden_out != NULL) {
    *hidden_out = hidden;
  }
  return hidden_decl;
}

typedef struct {
  ExpansionStatementASTNode* expansion;
} ExpansionJumpMarkData;

static void MarkExpansionLoopJumpVisitor(ASTNode* node, void* data, int child_id,
                                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL || data == NULL) {
    return;
  }
  ExpansionJumpMarkData* ctx = data;
  ASTNode* expansion = (ASTNode*)ctx->expansion;
  if (node->op == AST_OP(break)) {
    if (CXXEnclosingLoopOrSwitch(node) == expansion) {
      node->flags |= kASTExpansionLoopBreak;
    }
  } else if (node->op == AST_OP(continue)) {
    if (CXXEnclosingLoop(node) == expansion) {
      node->flags |= kASTExpansionLoopContinue;
    }
  }
}

void SemanticMarkExpansionLoopJumps(ExpansionStatementASTNode* expansion) {
  if (expansion == NULL || expansion->stmt == NULL) {
    return;
  }
  if ((expansion->base.flags & kASTExpansionJumpsMarked) != 0) {
    return;
  }
  expansion->base.flags |= kASTExpansionJumpsMarked;
  ExpansionJumpMarkData data = {.expansion = expansion};
  ASTNodeVisit(expansion->stmt, MarkExpansionLoopJumpVisitor, 0, &data);
}

static void DiagnoseExpansionLabelVisitor(ASTNode* node, void* data, int child_id,
                                          VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if (node->op == AST_OP(label) && data != NULL) {
    LabelASTNode* label = (LabelASTNode*)node;
    SemanticError((ASTNode*)label,
                  "A label declared in an expansion statement body is not "
                  "permitted");
  }
}

void SemanticDiagnoseExpansionEnclosedLabels(ExpansionStatementASTNode* expansion) {
  if (expansion == NULL || expansion->stmt == NULL) {
    return;
  }
  ASTNodeVisit(expansion->stmt, DiagnoseExpansionLabelVisitor, 0, expansion);
}

void SemanticAppendHiddenInitializerTemporaries(ASTNode* hidden_decl,
                                                Vector* statements) {
  AppendRangeForTemporaryDestructorStatements(hidden_decl, statements);
}
