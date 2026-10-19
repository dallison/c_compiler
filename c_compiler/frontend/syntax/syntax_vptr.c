//
//  syntax_vptr.c
//  c_compiler
//
//  Virtual table pointer (vptr) and virtual base pointer (vbptr)
//  initialization in constructors, and complete-object guards.
//

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "concepts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "macro_expansion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_internal.h"
#include "type_member.h"
#include "type_special_member.h"
#include "errors.h"
#include "compiler.h"
#include "module_identity.h"
#include "module_syntax.h"
#include "reflection_semantics.h"
#include "syntax_internal.h"

static TypeRecord* NewCXXStructType(Struct* str);

// Returns `(this + source_offset)` as a pointer to `source`, marked forced so
// template re-analysis does not rescale the offset.  NULL when `this` or
// `source` is unavailable.
static ASTNode* NewCXXVPtrReceiver(TypeRecord* func, Struct* source,
                                   int source_offset,
                                   SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL || source == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, NewCXXStructType(source));
  ASTNode* offset =
      NewIntConstantASTNode(source_offset,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), source_pointer, location, receiver,
                       offset);
  // Mark as a forced (already-analyzed) byte-offset pointer adjustment so it
  // survives template-instantiation re-analysis with its forced `source*` type
  // intact.  Otherwise `this + source_offset` is re-analyzed as the receiver's
  // own (element-scaled) pointer type and the `->__vptr` binds to the wrong
  // subobject offset -- for a secondary base at a non-zero offset this collapses
  // every vptr store onto offset 0 (see NewCXXVBPtrReceiver for the same fix).
  adjusted->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
  return adjusted;
}

Vector pending_vptr_inits;

bool pending_vptr_inits_initialized = false;

// Returns an expression statement `(adjusted_this)->__vptr = vtable + 2` for
// vtable entry `info`, or NULL when the constructor or vtable metadata is
// incomplete.
static ASTNode* NewCXXVPtrInitializer(TypeRecord* func, CXXVTableInfo* info,
                                      SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || info == NULL ||
      info->source == NULL || info->symbol == NULL) {
    return NULL;
  }
  StructMember* vptr_member = FindStructMemberByName(info->source, "__vptr");
  if (vptr_member == NULL) {
    return NULL;
  }
  ASTNode* receiver =
      NewCXXVPtrReceiver(func, info->source, info->source_offset, location);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNode* target =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver,
                       NewStringConstantASTNode(NewString("__vptr"), NULL,
                                                location));
  // The vtable begins with two RTTI header entries (offset_to_top and
  // &type_info); __vptr must point at the first function pointer, i.e. two
  // entries past the start of the table.
  ASTNode* value =
      NewBinaryASTNode(AST_OP(plus), NULL, location,
                       NewIdentifierASTNode(info->symbol, location),
                       NewIntConstantASTNode(
                           2, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), vptr_member->symbol->type,
                       location, target, value),
      location);
}

// Returns the subobject that physically owns the shared __vptr for `s`: the
// (unique) most-basic polymorphic class in `s`'s hierarchy that carries its own
// __vptr member.  Classes that derive from a polymorphic base (virtual or not)
// do not get their own vptr; they share the provider's.
static Struct* CXXFindVPtrProvider(Struct* s) {
  if (s == NULL) {
    return NULL;
  }
  if (s->vptr_member != NULL) {
    return s;
  }
  for (size_t i = 0; i < s->bases.length; i++) {
    CXXBaseSpecifier* base = s->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* p = CXXFindVPtrProvider(base->type->info.struct_info);
    if (p != NULL) {
      return p;
    }
  }
  for (size_t i = 0; i < s->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = s->virtual_bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* p = CXXFindVPtrProvider(base->type->info.struct_info);
    if (p != NULL) {
      return p;
    }
  }
  return NULL;
}

// Byte offset of the non-virtual base `to` within `from` (following only
// non-virtual base links), or false if `to` is not a non-virtual base.
static bool CXXNonVirtualBaseOffset(Struct* from, Struct* to, int inherited,
                                    int* out) {
  if (from == NULL || to == NULL) {
    return false;
  }
  if (from == to) {
    if (out != NULL) {
      *out = inherited;
    }
    return true;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (CXXNonVirtualBaseOffset(base->type->info.struct_info, to,
                                inherited + base->byte_offset, out)) {
      return true;
    }
  }
  return false;
}

// Computes the physical byte offset (within the complete object `owner`) of the
// __vptr slot that an initializer for `source` at `source_offset` would write.
// This lets us detect two initializers that target the same physical vptr, which
// happens when a polymorphic base is a (shared) virtual base: e.g. in a diamond
// `SS : IStream, OStream` where both virtually inherit polymorphic `Ios`, the
// SS/IStream/OStream vtables all target Ios's single shared vptr.  Only the
// first (most-derived, primary) initializer must survive; otherwise a secondary
// base's vtable overwrites the shared vptr and virtual dispatch (e.g. the
// destructor) receives a wrongly this-adjusted object.
static bool CXXVPtrPhysicalOffset(Struct* owner, Struct* source,
                                  int source_offset, int* out) {
  if (owner == NULL || source == NULL) {
    return false;
  }
  Struct* provider = CXXFindVPtrProvider(source);
  if (provider == NULL || provider->vptr_member == NULL) {
    return false;
  }
  int vptr_off = provider->vptr_member->byte_offset;
  // If the provider lives inside one of the complete object's virtual bases, the
  // vptr is shared and its physical offset is fixed by the complete-object
  // layout (independent of which base subobject we approached it through).
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* vb = owner->virtual_bases.value.p[i];
    if (vb->type == NULL || !TypeIsStructOrUnion(vb->type) ||
        vb->type->info.struct_info == NULL) {
      continue;
    }
    int within = 0;
    if (CXXNonVirtualBaseOffset(vb->type->info.struct_info, provider, 0,
                                &within)) {
      if (out != NULL) {
        *out = vb->byte_offset + within + vptr_off;
      }
      return true;
    }
  }
  // Otherwise the provider is reached from `source` through non-virtual bases.
  int within = 0;
  if (CXXNonVirtualBaseOffset(source, provider, 0, &within)) {
    if (out != NULL) {
      *out = source_offset + within + vptr_off;
    }
    return true;
  }
  return false;
}

// Returns the vtable symbol whose __vptr store targets physical offset
// `vptr_offset` in complete object `complete`, or NULL when none match.
Symbol* SyntaxCXXVTableForVPtrOffset(Struct* complete, int vptr_offset) {
  for (size_t i = 0; complete != NULL && i < complete->vtable_symbols.length;
       i++) {
    CXXVTableInfo* info = complete->vtable_symbols.value.p[i];
    int physical = 0;
    if (info != NULL && info->symbol != NULL &&
        CXXVPtrPhysicalOffset(complete, info->source, info->source_offset,
                              &physical) &&
        physical == vptr_offset) {
      return info->symbol;
    }
  }
  return NULL;
}

// Appends one __vptr assignment per distinct physical vptr slot of the
// constructor's class to `body`, skipping duplicate stores to a shared vptr.
void AppendCXXVPtrInitializers(TypeRecord* func, Vector* body,
                               SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || body == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  int* seen_offsets = NULL;
  size_t seen_count = 0;
  if (owner->vtable_symbols.length > 0) {
    seen_offsets = malloc(owner->vtable_symbols.length * sizeof(int));
  }
  for (size_t i = 0; i < owner->vtable_symbols.length; i++) {
    CXXVTableInfo* info = owner->vtable_symbols.value.p[i];
    int phys = 0;
    if (seen_offsets != NULL &&
        CXXVPtrPhysicalOffset(owner, info->source, info->source_offset,
                              &phys)) {
      bool already = false;
      for (size_t j = 0; j < seen_count; j++) {
        if (seen_offsets[j] == phys) {
          already = true;
          break;
        }
      }
      if (already) {
        continue;
      }
      seen_offsets[seen_count++] = phys;
    }
    ASTNode* init = NewCXXVPtrInitializer(func, info, location);
    if (init != NULL) {
      VectorAppend(body, init);
    }
  }
  free(seen_offsets);
}

// `receiver->__vptr = vtable + 2;` as built by NewCXXVPtrInitializer, before
// or after analysis resolves the member.
static bool IsCXXVPtrStore(ASTNode* stmt) {
  if (stmt == NULL || stmt->op != AST_OP(expr)) {
    return false;
  }
  ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
  if (expr == NULL || expr->op != AST_OP(assign)) {
    return false;
  }
  ASTNode* target = ((BinaryASTNode*)expr)->left;
  if (target == NULL || target->op != AST_OP(arrow)) {
    return false;
  }
  ASTNode* member = ((BinaryASTNode*)target)->right;
  if (member != NULL && member->op == AST_OP(string)) {
    String* name = ((ConstantASTNode*)member)->value.string;
    return name != NULL && StringEqual(name, "__vptr");
  }
  if (member != NULL && member->op == AST_OP(structmember)) {
    StructMember* field = ((StructMemberASTNode*)member)->member;
    return field != NULL && field->symbol != NULL &&
           StringEqual(&field->symbol->name, "__vptr");
  }
  return false;
}

// Removes existing __vptr store statements from `body`, then inserts fresh ones
// at the first removed index.  Sets `*first_out` to that index and returns the
// number inserted, or 0 when vtables are not registered or no stores were found.
size_t SyntaxReplaceCXXVPtrInitializers(TypeRecord* func, Vector* body,
                                        size_t* first_out) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL ||
      !func->info.function.cxx_member_owner->vtables_registered ||
      body == NULL) {
    return 0;
  }
  size_t first = body->length;
  SourceLocation location = SOURCE_LOCATION_MISSING;
  size_t out = 0;
  for (size_t i = 0; i < body->length; i++) {
    ASTNode* stmt = body->value.p[i];
    if (IsCXXVPtrStore(stmt)) {
      if (first == body->length) {
        first = out;
        location = stmt->location;
      }
      ASTNodeDelete(stmt);
      continue;
    }
    body->value.p[out++] = stmt;
  }
  if (first == body->length) {
    return 0;
  }
  body->length = out;
  Vector* stores = NewVector();
  AppendCXXVPtrInitializers(func, stores, location);
  for (size_t i = 0; i < stores->length; i++) {
    VectorInsertOrAppend(body, first + i, stores->value.p[i]);
  }
  size_t count = stores->length;
  VectorDelete(stores);
  *first_out = first;
  return count;
}

// Returns a plain struct type for layout and pointer arithmetic on `str`.
static TypeRecord* NewCXXStructType(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(type, str);
  TypeRecordCalculateSize(type);
  return type;
}

// Returns `(this + info->source_offset)` as a pointer to `info->source`, with
// forced type adjustment like NewCXXVPtrReceiver.  NULL on failure.
static ASTNode* NewCXXVBPtrReceiver(TypeRecord* func,
                                    CXXVBTableInfo* info,
                                    SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL || info == NULL || info->source == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, NewCXXStructType(info->source));
  ASTNode* offset =
      NewIntConstantASTNode(info->source_offset,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), source_pointer, location, receiver,
                       offset);
  adjusted->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
  return adjusted;
}

// Returns `(adjusted_this)->__vbptr = vbtable` for virtual-base table `info`,
// or NULL when the constructor or vbtable metadata is incomplete.
static ASTNode* NewCXXVBPtrInitializer(TypeRecord* func,
                                       CXXVBTableInfo* info,
                                       SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || info == NULL ||
      info->symbol == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewCXXVBPtrReceiver(func, info, location);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNode* target =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver,
                       NewStringConstantASTNode(NewString("__vbptr"), NULL,
                                                location));
  ASTNode* value = NewIdentifierASTNode(info->symbol, location);
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), info->symbol->type, location, target,
                       value),
      location);
}

// Appends one __vbptr assignment per registered vbtable of the constructor's
// class to `body`.
void AppendCXXVBPtrInitializers(TypeRecord* func, Vector* body,
                                SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || body == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = owner->vbtable_symbols.value.p[i];
    ASTNode* init = NewCXXVBPtrInitializer(func, info, location);
    if (init != NULL) {
      VectorAppend(body, init);
    }
  }
}

// Returns the complete-object guard parameter (prototype slot 1) of constructor
// or destructor `func` when the class has virtual bases, else NULL.
static Symbol* CXXCompleteObjectSymbolFromFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2 ||
      func->info.function.cxx_member_owner == NULL ||
      !StructHasVirtualBases(func->info.function.cxx_member_owner) ||
      (!func->info.function.is_constructor &&
       !func->info.function.is_destructor)) {
    return NULL;
  }
  return func->info.function.prototype.value.p[1];
}

// Wraps `statements` in `if (complete_object) { ... }` when the function has a
// complete-object guard, otherwise returns a compound of `statements`.  Takes
// ownership of `statements` and returns NULL when it is empty.
ASTNode* NewCXXCompleteObjectGuardedStatement(TypeRecord* func,
                                                    Vector* statements,
                                                    SourceLocation location) {
  if (statements == NULL || statements->length == 0) {
    if (statements != NULL) {
      VectorDelete(statements);
    }
    return NULL;
  }
  ASTNode* body = NewCompoundStatementASTNode(statements, location);
  Symbol* complete_object = CXXCompleteObjectSymbolFromFunction(func);
  if (complete_object == NULL) {
    return body;
  }
  return NewIfStatementASTNode(NewIdentifierASTNode(complete_object, location),
                               body, NULL, false, location);
}

// Inserts a complete-object-guarded compound of `statements` into `body` at
// `*insert_at`, then advances `*insert_at` past the insertion.
void InsertCXXCompleteObjectGuardedStatements(TypeRecord* func,
                                                    Vector* body,
                                                    size_t* insert_at,
                                                    Vector* statements,
                                                    SourceLocation location) {
  ASTNode* guarded =
      NewCXXCompleteObjectGuardedStatement(func, statements, location);
  if (guarded == NULL) {
    return;
  }
  VectorInsertOrAppend(body, *insert_at, guarded);
  (*insert_at)++;
}

// For pending vptr/vbptr work queued for `owner`, splices deferred __vbptr and
// __vptr initializer statements into each constructor body and frees matching
// queue entries; retains entries for other classes.
void SyntaxFlushPendingVPtrInitializers(Struct* owner) {
  if (!pending_vptr_inits_initialized || owner == NULL) {
    return;
  }
  size_t out = 0;
  for (size_t i = 0; i < pending_vptr_inits.length; i++) {
    PendingVPtrInit* pending = pending_vptr_inits.value.p[i];
    if (pending->owner != owner) {
      // Keep entries for other (e.g. enclosing) classes still being parsed.
      pending_vptr_inits.value.p[out++] = pending;
      continue;
    }
    ASTNode* body = pending->func->info.function.body;
    if (body != NULL && body->op == AST_OP(compound)) {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)body;
      bool inserted_any = false;
      // __vbptr initializers share the same registration dependency as the
      // __vptr stores: when the preamble was built the class's vbtables had not
      // been registered, so AppendCXXVBPtrInitializers produced nothing and the
      // eager preamble emission dropped them.  Re-emit them here.  The first
      // batch must run before any virtual base is constructed, because the
      // derived-to-virtual-base pointer conversion reads __vbptr; insert it as a
      // complete-object-guarded block at the very front, ahead of the existing
      // virtual-base construction block (also at the front).
      size_t vptr_index = pending->index;
      Vector* vbptr_inits = NewVector();
      AppendCXXVBPtrInitializers(pending->func, vbptr_inits, pending->location);
      if (vbptr_inits->length > 0) {
        ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
            pending->func, vbptr_inits, pending->location);
        if (guarded != NULL) {
          VectorInsertOrAppend(compound->statements, 0, guarded);
          guarded->parent = &compound->base;
          vptr_index++;
          inserted_any = true;
        }
      } else {
        VectorDelete(vbptr_inits);
      }

      Vector* vptr_initializers = NewVector();
      AppendCXXVPtrInitializers(pending->func, vptr_initializers,
                                pending->location);
      // VectorInsertOrAppend handles index == length (e.g. an empty body whose
      // only statements are the deferred __vptr stores), which the strict
      // VectorInsertBefore inside CompoundASTNodeInsertStatement does not.
      size_t at = vptr_index;
      for (size_t j = 0; j < vptr_initializers->length; j++) {
        ASTNode* stmt = vptr_initializers->value.p[j];
        VectorInsertOrAppend(compound->statements, at, stmt);
        stmt->parent = &compound->base;
        at++;
      }
      if (vptr_initializers->length > 0) {
        inserted_any = true;
      }
      VectorDelete(vptr_initializers);

      // A base-class constructor may have overwritten __vbptr with its own
      // (base-subobject) vbtable, so restore the most-derived vbtables after the
      // base constructors and __vptr stores, matching the eager preamble path.
      Vector* vbptr_restores = NewVector();
      AppendCXXVBPtrInitializers(pending->func, vbptr_restores,
                                 pending->location);
      if (vbptr_restores->length > 0) {
        ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
            pending->func, vbptr_restores, pending->location);
        if (guarded != NULL) {
          VectorInsertOrAppend(compound->statements, at, guarded);
          guarded->parent = &compound->base;
          inserted_any = true;
        }
      } else {
        VectorDelete(vbptr_restores);
      }

      if (inserted_any) {
        for (size_t k = 0; k < compound->statements->length; k++) {
          ((ASTNode*)compound->statements->value.p[k])->child_id = (int)k;
        }
      }
    }
    free(pending);
  }
  pending_vptr_inits.length = out;
}
