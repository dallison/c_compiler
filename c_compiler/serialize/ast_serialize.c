//
//  ast_serialize.c
//  c_compiler
//
//  Serialization of AST nodes.  Nodes are a single pooled kind
//  (kSerialKindAST); the concrete struct is discriminated by an ASTNodeShape
//  written into every record (field 2).  On read, ASTNodeAllocForShape creates
//  the correctly-typed node with the right virtual table, and the second pass
//  fills in the fields.
//
//  Field numbering:
//    * Base ASTNode fields occupy 1..9 (reserved range 1..15).
//    * Subtype-specific fields start at 16 and are numbered per shape; the same
//      number may be reused by different shapes since the shape discriminates.
//
//  Codegen/transient fields (IRNode* labels, saved_sp, switch analytics, DIEs)
//  are omitted and recomputed after load.
//

#include <stdlib.h>

#include "ast.h"
#include "constraint_serialize.h"
#include "reflection.h"
#include "serialize_common.h"
#include "symbol.h"
#include "type.h"

// Base field numbers.
enum {
  kAST_op = 1,
  kAST_shape = 2,
  kAST_id = 3,
  kAST_flags = 4,
  kAST_type = 5,
  kAST_value_category = 6,
  kAST_parent = 7,
  kAST_child_id = 8,
  kAST_location = 9,
};

static const WireFieldDesc kASTBaseFields[] = {
    {kAST_op, "op"},
    {kAST_shape, "shape"},
    {kAST_id, "id"},
    {kAST_flags, "flags"},
    {kAST_type, "type"},
    {kAST_value_category, "value_category"},
    {kAST_parent, "parent"},
    {kAST_child_id, "child_id"},
    {kAST_location, "location"},
};

// ---------------------------------------------------------------------------
// Small helpers for nullable Vector* of AST nodes.
// ---------------------------------------------------------------------------
static void WriteASTVectorPtr(SerializeContext* ctx, WireBuffer* buf, int field,
                              Vector* v) {
  if (v == NULL) {
    return;
  }
  SWriteRefVector(ctx, buf, field, kSerialKindAST, v);
}

static Vector* ReadASTVectorPtr(DeserializeContext* ctx, WireBuffer* buf) {
  Vector* v = NewVector();
  SReadRefVector(ctx, buf, kSerialKindAST, v);
  return v;
}

// ---------------------------------------------------------------------------
// AsmOperand vector (inline sub-message).
// ---------------------------------------------------------------------------
enum {
  kAsmOp_constraint = 1,
  kAsmOp_name = 2,
  kAsmOp_expr = 3,
  kAsmOp_is_output = 4,
  kAsmOp_is_readwrite = 5,
  kAsmOp_is_early_clobber = 6,
};

static void WriteAsmOperandVector(SerializeContext* ctx, WireBuffer* buf,
                                  int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  WireWriteRawVarint(&tmp, (uint64_t)v->length);
  for (size_t i = 0; i < v->length; i++) {
    AsmOperand* op = (AsmOperand*)VectorGet(v, i);
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    SWriteStringVal(ctx, &elem, kAsmOp_constraint, &op->constraint);
    SWriteStringVal(ctx, &elem, kAsmOp_name, &op->name);
    SWriteRef(ctx, &elem, kAsmOp_expr, kSerialKindAST, op->expr);
    WireWriteBool(&elem, kAsmOp_is_output, op->is_output);
    WireWriteBool(&elem, kAsmOp_is_readwrite, op->is_readwrite);
    WireWriteBool(&elem, kAsmOp_is_early_clobber, op->is_early_clobber);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadAsmOperandVector(DeserializeContext* ctx, WireBuffer* buf,
                                 Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(buf, &data, &len)) {
    return;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count;
  if (!WireReadRawVarint(&sub, &count)) {
    return;
  }
  for (uint64_t i = 0; i < count; i++) {
    const void* elem;
    size_t elen;
    if (!WireReadBytes(&sub, &elem, &elen)) {
      return;
    }
    WireBuffer er;
    WireBufferInitReader(&er, elem, elen);
    AsmOperand* op = NewAsmOperand("", "", NULL, false);
    while (!WireBufferEof(&er) && !WireBufferHasError(&er)) {
      int field;
      WireType wt;
      if (!WireReadTag(&er, &field, &wt)) {
        break;
      }
      switch (field) {
        case kAsmOp_constraint:
          SReadStringVal(ctx, &er, &op->constraint);
          break;
        case kAsmOp_name:
          SReadStringVal(ctx, &er, &op->name);
          break;
        case kAsmOp_expr:
          op->expr = (ASTNode*)SReadRef(ctx, &er, kSerialKindAST);
          break;
        case kAsmOp_is_output:
          WireReadBool(&er, &op->is_output);
          break;
        case kAsmOp_is_readwrite:
          WireReadBool(&er, &op->is_readwrite);
          break;
        case kAsmOp_is_early_clobber:
          WireReadBool(&er, &op->is_early_clobber);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    VectorAppend(out, op);
  }
}

// ---------------------------------------------------------------------------
// Designator vector (inline sub-message).
// ---------------------------------------------------------------------------
enum {
  kDesig_designator_type = 1,
  kDesig_type = 2,
  kDesig_array_index_end = 3,
  kDesig_is_resolved_member = 4,
  kDesig_array_index = 5,
  kDesig_struct_member_name = 6,
  kDesig_struct_member = 7,
};

static void WriteDesignatorVector(SerializeContext* ctx, WireBuffer* buf,
                                  int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    Designator* d = (Designator*)VectorGet(v, i);
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    WireWriteInt32(&elem, kDesig_designator_type, (int32_t)d->designator_type);
    SWriteRef(ctx, &elem, kDesig_type, kSerialKindType, d->type);
    WireWriteInt32(&elem, kDesig_array_index_end, d->array_index_end);
    WireWriteBool(&elem, kDesig_is_resolved_member, d->is_resolved_member);
    if (d->designator_type == kDesignatorArray) {
      WireWriteInt32(&elem, kDesig_array_index, d->value.array_index);
    } else if (d->designator_type == kDesignatorStruct) {
      if (d->is_resolved_member) {
        SWriteRef(ctx, &elem, kDesig_struct_member, kSerialKindStructMember,
                  d->value.struct_member);
      } else {
        SWriteStringPtr(ctx, &elem, kDesig_struct_member_name,
                        d->value.struct_member_name);
      }
    }
    // kDesignatorBase (CXXBaseSpecifier*) is not pooled and is not serialized.
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static Vector* ReadDesignatorVector(DeserializeContext* ctx, WireBuffer* buf) {
  const void* data;
  size_t len;
  if (!WireReadBytes(buf, &data, &len)) {
    return NULL;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count;
  if (!WireReadRawVarint(&sub, &count)) {
    return NULL;
  }
  Vector* out = NewVector();
  for (uint64_t i = 0; i < count; i++) {
    const void* elem;
    size_t elen;
    if (!WireReadBytes(&sub, &elem, &elen)) {
      break;
    }
    WireBuffer er;
    WireBufferInitReader(&er, elem, elen);
    Designator* d = (Designator*)calloc(1, sizeof(Designator));
    while (!WireBufferEof(&er) && !WireBufferHasError(&er)) {
      int field;
      WireType wt;
      if (!WireReadTag(&er, &field, &wt)) {
        break;
      }
      switch (field) {
        case kDesig_designator_type: {
          int32_t val;
          WireReadInt32(&er, &val);
          d->designator_type = (DesignatorType)val;
          break;
        }
        case kDesig_type:
          d->type = (TypeRecord*)SReadRef(ctx, &er, kSerialKindType);
          TypeRecordIncRef(d->type);
          break;
        case kDesig_array_index_end:
          WireReadInt32(&er, &d->array_index_end);
          break;
        case kDesig_is_resolved_member:
          WireReadBool(&er, &d->is_resolved_member);
          break;
        case kDesig_array_index:
          WireReadInt32(&er, &d->value.array_index);
          break;
        case kDesig_struct_member_name:
          d->value.struct_member_name = SReadStringPtr(ctx, &er);
          break;
        case kDesig_struct_member:
          d->value.struct_member =
              (StructMember*)SReadRef(ctx, &er, kSerialKindStructMember);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    VectorAppend(out, d);
  }
  return out;
}

// ---------------------------------------------------------------------------
// Base fields.
// ---------------------------------------------------------------------------
static void WriteASTBase(SerializeContext* ctx, WireBuffer* buf, ASTNode* n,
                         ASTNodeShape shape) {
  WireWriteInt32(buf, kAST_op, (int32_t)n->op);
  WireWriteInt32(buf, kAST_shape, (int32_t)shape);
  WireWriteInt32(buf, kAST_id, n->id);
  WireWriteUint64(buf, kAST_flags, n->flags);
  SWriteRef(ctx, buf, kAST_type, kSerialKindType, n->type);
  WireWriteInt32(buf, kAST_value_category, (int32_t)n->value_category);
  // Parent is a weak/back reference.  A valid parent reached from the root is
  // already interned before its child is drained; do not intern through this
  // edge because detached template fragments can retain stale former parents.
  SerialHandle parent_handle = kSerialNullHandle;
  if (n->parent != NULL) {
    void* found =
        MapFindPointerKey(&ctx->handle_maps[kSerialKindAST], n->parent);
    if (found != NULL) {
      parent_handle = (SerialHandle)(intptr_t)found;
    }
  }
  WireWriteVarint(buf, kAST_parent, parent_handle);
  WireWriteInt32(buf, kAST_child_id, n->child_id);
  WireWriteUint64(buf, kAST_location, (uint64_t)n->location);
}

// Returns true if `field` was a base field and was handled.
static bool ReadASTBaseField(DeserializeContext* ctx, WireBuffer* buf,
                             ASTNode* n, int field) {
  switch (field) {
    case kAST_op: {
      int32_t v;
      WireReadInt32(buf, &v);
      n->op = (ASTOpcode)v;
      return true;
    }
    case kAST_shape: {
      int32_t v;
      WireReadInt32(buf, &v);  // Already consumed by alloc; ignore.
      (void)v;
      return true;
    }
    case kAST_id:
      WireReadInt32(buf, &n->id);
      return true;
    case kAST_flags:
      WireReadUint64(buf, &n->flags);
      return true;
    case kAST_type:
      n->type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
      // AST nodes replace and release their analyzed types while imported
      // templates are instantiated.  Rebuild this owning reference so a clone
      // cannot tear down the source node's shared type chain.
      TypeRecordIncRef(n->type);
      return true;
    case kAST_value_category: {
      int32_t v;
      WireReadInt32(buf, &v);
      n->value_category = (ASTValueCategory)v;
      return true;
    }
    case kAST_parent:
      n->parent = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
      return true;
    case kAST_child_id:
      WireReadInt32(buf, &n->child_id);
      return true;
    case kAST_location: {
      uint64_t v;
      WireReadUint64(buf, &v);
      n->location = (SourceLocation)v;
      return true;
    }
    default:
      return false;
  }
}

// ---------------------------------------------------------------------------
// Subtype writers.  Field numbers begin at 16.
// ---------------------------------------------------------------------------
static void WriteASTSub(SerializeContext* ctx, WireBuffer* buf, ASTNode* n,
                        ASTNodeShape shape) {
  switch (shape) {
    case kASTShapeUnary: {
      UnaryASTNode* u = (UnaryASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, u->sub);
      break;
    }
    case kASTShapeBinary: {
      BinaryASTNode* b = (BinaryASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, b->left);
      SWriteRef(ctx, buf, 17, kSerialKindAST, b->right);
      break;
    }
    case kASTShapeInlineCall: {
      InlineCallASTNode* c = (InlineCallASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, c->inlined);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->ret_value);
      break;
    }
    case kASTShapeVector: {
      VectorASTNode* v = (VectorASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, v->left);
      WriteASTVectorPtr(ctx, buf, 17, v->children);
      break;
    }
    case kASTShapeRequiresExpr: {
      RequiresExpressionASTNode* r = (RequiresExpressionASTNode*)n;
      SerialWriteConstraint(ctx, buf, 16, r->constraint);
      break;
    }
    case kASTShapeIdentifier: {
      IdentifierASTNode* id = (IdentifierASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindSymbol, id->symbol);
      if (id->template_arguments != NULL) {
        SerialWriteTemplateArgumentVector(ctx, buf, 17,
                                          id->template_arguments);
      }
      break;
    }
    case kASTShapeStructMember: {
      StructMemberASTNode* sm = (StructMemberASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindStructMember, sm->member);
      WireWriteInt32(buf, 17, (int32_t)sm->access);
      WireWriteInt32(buf, 18, sm->byte_offset);
      if (sm->template_arguments != NULL) {
        SerialWriteTemplateArgumentVector(ctx, buf, 19,
                                          sm->template_arguments);
      }
      break;
    }
    case kASTShapeConstant: {
      ConstantASTNode* c = (ConstantASTNode*)n;
      if (n->op == AST_OP(string) || n->op == AST_OP(string_wide)) {
        SWriteStringPtr(ctx, buf, 18, c->value.string);
      } else if (n->op == AST_OP(fnumber)) {
        WireWriteDouble(buf, 17, c->value.fvalue);
      } else {
        WireWriteInt64(buf, 16, c->value.ivalue);
      }
      if (c->template_arguments != NULL) {
        SerialWriteTemplateArgumentVector(ctx, buf, 19,
                                          c->template_arguments);
      }
      break;
    }
    case kASTShapeCast: {
      CastASTNode* c = (CastASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindType, c->cast_type);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->expr);
      WireWriteInt32(buf, 18, (int32_t)c->kind);
      WireWriteBool(buf, 19, c->dynamic_runtime);
      break;
    }
    case kASTShapeSizeof: {
      SizeofASTNode* s = (SizeofASTNode*)n;
      WireWriteInt64(buf, 16, s->base.value.ivalue);
      SWriteRef(ctx, buf, 20, kSerialKindAST, s->expr);
      SWriteRef(ctx, buf, 21, kSerialKindType, s->type_operand);
      WireWriteBool(buf, 22, s->is_pack_size);
      break;
    }
    case kASTShapeTypeid: {
      TypeidASTNode* t = (TypeidASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, t->expr);
      SWriteRef(ctx, buf, 17, kSerialKindType, t->operand_type);
      break;
    }
    case kASTShapeReflection: {
      ReflectionASTNode* r = (ReflectionASTNode*)n;
      WireWriteInt32(buf, 16, (int32_t)r->operand_kind);
      SWriteRef(ctx, buf, 17, kSerialKindAST, r->operand);
      SWriteRef(ctx, buf, 18, kSerialKindType, r->operand_type);
      SWriteRef(ctx, buf, 19, kSerialKindNamespace, r->namespace_);
      if (r->value != NULL) {
        WireWriteInt32(buf, 20, (int32_t)r->value->kind);
        SWriteRef(ctx, buf, 21, kSerialKindType,
                  r->value->reflected_type);
        SWriteRef(ctx, buf, 22, kSerialKindSymbol, r->value->symbol);
        SWriteRef(ctx, buf, 23, kSerialKindStructMember,
                  r->value->member);
        SWriteRef(ctx, buf, 24, kSerialKindNamespace,
                  r->value->namespace_);
        SWriteRef(ctx, buf, 25, kSerialKindStruct,
                  r->value->parent_class);
        WireWriteUint64(buf, 26, (uint64_t)r->value->base_index);
        WireWriteUint64(buf, 27, (uint64_t)r->value->location);
      }
      break;
    }
    case kASTShapeSplice: {
      SpliceASTNode* s = (SpliceASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, s->reflection);
      WireWriteInt32(buf, 17, (int32_t)s->context);
      break;
    }
    case kASTShapeMacro: {
      MacroNameASTNode* m = (MacroNameASTNode*)n;
      SWriteStringVal(ctx, buf, 16, &m->macro_name);
      break;
    }
    case kASTShapeExprStmt: {
      ExpressionStatementASTNode* e = (ExpressionStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, e->expr);
      break;
    }
    case kASTShapeStaticAssert: {
      StaticAssertASTNode* a = (StaticAssertASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, a->expr);
      SWriteStringVal(ctx, buf, 17, &a->message);
      SWriteRef(ctx, buf, 18, kSerialKindAST, a->message_expr);
      break;
    }
    case kASTShapeContractAssert: {
      ContractAssertASTNode* a = (ContractAssertASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, a->predicate);
      SerialWriteAttributeVector(ctx, buf, 17, &a->attributes);
      break;
    }
    case kASTShapeIf: {
      IfStatementASTNode* i = (IfStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, i->cond);
      SWriteRef(ctx, buf, 17, kSerialKindAST, i->if_part);
      SWriteRef(ctx, buf, 18, kSerialKindAST, i->else_part);
      WireWriteBool(buf, 19, i->is_constexpr);
      WireWriteBool(buf, 20, i->is_consteval);
      WireWriteBool(buf, 21, i->consteval_negated);
      break;
    }
    case kASTShapeCombined: {
      CombinedStatementASTNode* c = (CombinedStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, c->cond);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->stmt);
      break;
    }
    case kASTShapeThrow: {
      ThrowASTNode* t = (ThrowASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, t->expr);
      break;
    }
    case kASTShapeCompound: {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)n;
      WriteASTVectorPtr(ctx, buf, 16, c->statements);
      SWriteRef(ctx, buf, 17, kSerialKindAST, (ASTNode*)c->low_pc);
      SWriteRef(ctx, buf, 18, kSerialKindAST, (ASTNode*)c->high_pc);
      break;
    }
    case kASTShapeCatch: {
      CatchASTNode* c = (CatchASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindSymbol, c->symbol);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->stmt);
      WireWriteBool(buf, 18, c->is_catch_all);
      break;
    }
    case kASTShapeTry: {
      TryASTNode* t = (TryASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, t->try_stmt);
      WriteASTVectorPtr(ctx, buf, 17, t->catches);
      break;
    }
    case kASTShapeFor: {
      ForStatementASTNode* f = (ForStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, f->c1);
      SWriteRef(ctx, buf, 17, kSerialKindAST, f->c2);
      SWriteRef(ctx, buf, 18, kSerialKindAST, f->c3);
      SWriteRef(ctx, buf, 19, kSerialKindAST, f->stmt);
      break;
    }
    case kASTShapeExpansionFor: {
      ExpansionStatementASTNode* e = (ExpansionStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, e->init_stmt);
      WireWriteInt32(buf, 17, (int32_t)e->item_kind);
      SWriteRef(ctx, buf, 18, kSerialKindSymbol, e->item_symbol);
      SWriteRef(ctx, buf, 19, kSerialKindType, e->binding_type);
      if (e->binding_names != NULL) {
        SWriteStringVector(ctx, buf, 20, e->binding_names);
      }
      if (e->binding_symbols != NULL) {
        SWriteRefVector(ctx, buf, 21, kSerialKindSymbol, e->binding_symbols);
      }
      WireWriteInt32(buf, 22, e->binding_pack_index);
      WireWriteInt32(buf, 23, (int32_t)e->init_kind);
      SWriteRef(ctx, buf, 24, kSerialKindAST, e->initializer);
      SWriteRef(ctx, buf, 25, kSerialKindAST, e->stmt);
      break;
    }
    case kASTShapeVarDecl: {
      VariableDeclarationASTNode* v = (VariableDeclarationASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindSymbol, v->symbol);
      SWriteRef(ctx, buf, 17, kSerialKindAST, v->initializer);
      SWriteRef(ctx, buf, 18, kSerialKindSymbol, v->local_static_guard);
      WireWriteInt64(buf, 19, v->local_static_init_kind);
      break;
    }
    case kASTShapeDeclList: {
      DeclarationListASTNode* d = (DeclarationListASTNode*)n;
      WriteASTVectorPtr(ctx, buf, 16, d->declarations);
      break;
    }
    case kASTShapeCaseLabel: {
      CaseLabelASTNode* c = (CaseLabelASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, c->expr);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->stmt);
      WireWriteInt64(buf, 18, c->value);
      break;
    }
    case kASTShapeSwitch: {
      SwitchStatementASTNode* s = (SwitchStatementASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, s->expr);
      SWriteRef(ctx, buf, 17, kSerialKindAST, s->stmt);
      SWriteRefVector(ctx, buf, 18, kSerialKindAST, &s->cases);
      SWriteRef(ctx, buf, 19, kSerialKindAST, (ASTNode*)s->default_node);
      break;
    }
    case kASTShapeLabel: {
      LabelASTNode* l = (LabelASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, l->stmt);
      SWriteStringVal(ctx, buf, 17, &l->name);
      WireWriteBool(buf, 18, l->named);
      break;
    }
    case kASTShapeAsm: {
      AsmASTNode* a = (AsmASTNode*)n;
      SWriteStringPtr(ctx, buf, 16, a->text);
      WireWriteBool(buf, 17, a->is_volatile);
      WireWriteBool(buf, 18, a->is_goto);
      WriteAsmOperandVector(ctx, buf, 19, &a->outputs);
      WriteAsmOperandVector(ctx, buf, 20, &a->inputs);
      SWriteStringVector(ctx, buf, 21, &a->clobbers);
      SWriteStringVector(ctx, buf, 22, &a->labels);
      SWriteRefVector(ctx, buf, 23, kSerialKindAST, &a->label_nodes);
      break;
    }
    case kASTShapeGoto: {
      GotoStatementASTNode* g = (GotoStatementASTNode*)n;
      SWriteStringPtr(ctx, buf, 16, g->label_name);
      SWriteRef(ctx, buf, 17, kSerialKindAST, g->label);
      SWriteRef(ctx, buf, 18, kSerialKindAST, g->lca);
      break;
    }
    case kASTShapePtrScale: {
      PtrScaleASTNode* p = (PtrScaleASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindType, p->ref_type);
      WireWriteInt32(buf, 17, (int32_t)p->scale_op);
      SWriteRef(ctx, buf, 18, kSerialKindAST, p->expr);
      break;
    }
    case kASTShapeExprInit: {
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, e->expr);
      break;
    }
    case kASTShapeBracedInit: {
      BracedInitializerASTNode* b = (BracedInitializerASTNode*)n;
      WriteASTVectorPtr(ctx, buf, 16, b->initializers);
      break;
    }
    case kASTShapeDesignatedInit: {
      DesignatedInitializerASTNode* d = (DesignatedInitializerASTNode*)n;
      WriteDesignatorVector(ctx, buf, 16, d->designators);
      SWriteRef(ctx, buf, 17, kSerialKindAST, d->init);
      break;
    }
    case kASTShapeCompoundLiteral: {
      CompoundLiteralASTNode* c = (CompoundLiteralASTNode*)n;
      SWriteRef(ctx, buf, 16, kSerialKindAST, c->sym);
      SWriteRef(ctx, buf, 17, kSerialKindAST, c->initializer);
      break;
    }
    case kASTShapeBase:
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Subtype readers.
// ---------------------------------------------------------------------------
static void ReadASTSubField(DeserializeContext* ctx, WireBuffer* buf,
                            ASTNode* n, ASTNodeShape shape, int field,
                            WireType wt) {
  switch (shape) {
    case kASTShapeUnary: {
      UnaryASTNode* u = (UnaryASTNode*)n;
      if (field == 16) {
        u->sub = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeBinary: {
      BinaryASTNode* b = (BinaryASTNode*)n;
      if (field == 16) {
        b->left = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        b->right = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeInlineCall: {
      InlineCallASTNode* c = (InlineCallASTNode*)n;
      if (field == 16) {
        c->inlined = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        c->ret_value = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeVector: {
      VectorASTNode* v = (VectorASTNode*)n;
      if (field == 16) {
        v->left = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        v->children = ReadASTVectorPtr(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeRequiresExpr: {
      RequiresExpressionASTNode* r = (RequiresExpressionASTNode*)n;
      if (field == 16) {
        r->constraint = SerialReadConstraint(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeIdentifier: {
      IdentifierASTNode* id = (IdentifierASTNode*)n;
      if (field == 16) {
        id->symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 17) {
        id->template_arguments =
            SerialReadTemplateArgumentVector(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeStructMember: {
      StructMemberASTNode* sm = (StructMemberASTNode*)n;
      if (field == 16) {
        StructMember* member =
            (StructMember*)SReadRef(ctx, buf, kSerialKindStructMember);
        StructMemberASTNodeSetMember(sm, member);
        return;
      }
      if (field == 17) {
        int32_t v;
        WireReadInt32(buf, &v);
        sm->access = (CXXAccess)v;
        return;
      }
      if (field == 18) {
        WireReadInt32(buf, &sm->byte_offset);
        return;
      }
      if (field == 19) {
        sm->template_arguments =
            SerialReadTemplateArgumentVector(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeConstant: {
      ConstantASTNode* c = (ConstantASTNode*)n;
      if (field == 16) {
        WireReadInt64(buf, &c->value.ivalue);
        return;
      }
      if (field == 17) {
        WireReadDouble(buf, &c->value.fvalue);
        return;
      }
      if (field == 18) {
        c->value.string = SReadStringPtr(ctx, buf);
        return;
      }
      if (field == 19) {
        c->template_arguments =
            SerialReadTemplateArgumentVector(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeCast: {
      CastASTNode* c = (CastASTNode*)n;
      if (field == 16) {
        c->cast_type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(c->cast_type);
        return;
      }
      if (field == 17) {
        c->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        int32_t v;
        WireReadInt32(buf, &v);
        c->kind = (CastKind)v;
        return;
      }
      if (field == 19) {
        WireReadBool(buf, &c->dynamic_runtime);
        return;
      }
      break;
    }
    case kASTShapeSizeof: {
      SizeofASTNode* s = (SizeofASTNode*)n;
      if (field == 16) {
        WireReadInt64(buf, &s->base.value.ivalue);
        return;
      }
      if (field == 20) {
        s->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 21) {
        s->type_operand = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(s->type_operand);
        return;
      }
      if (field == 22) {
        WireReadBool(buf, &s->is_pack_size);
        return;
      }
      break;
    }
    case kASTShapeTypeid: {
      TypeidASTNode* t = (TypeidASTNode*)n;
      if (field == 16) {
        t->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        t->operand_type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(t->operand_type);
        return;
      }
      break;
    }
    case kASTShapeReflection: {
      ReflectionASTNode* r = (ReflectionASTNode*)n;
      if (field == 16) {
        int32_t kind = 0;
        WireReadInt32(buf, &kind);
        r->operand_kind = (ReflectionOperandKind)kind;
        return;
      }
      if (field == 17) {
        r->operand = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        r->operand_type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(r->operand_type);
        return;
      }
      if (field == 19) {
        r->namespace_ =
            (Namespace*)SReadRef(ctx, buf, kSerialKindNamespace);
        return;
      }
      if (field == 20) {
        int32_t kind = 0;
        WireReadInt32(buf, &kind);
        r->value = ReflectionCreateDeserialized(
            (ReflectionEntityKind)kind, n->location);
        return;
      }
      if (r->value == NULL && field >= 21 && field <= 27) {
        r->value = ReflectionCreateDeserialized(kReflectionInvalid,
                                                n->location);
      }
      if (field == 21) {
        r->value->reflected_type =
            (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(r->value->reflected_type);
        return;
      }
      if (field == 22) {
        r->value->symbol =
            (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 23) {
        r->value->member =
            (StructMember*)SReadRef(ctx, buf, kSerialKindStructMember);
        return;
      }
      if (field == 24) {
        r->value->namespace_ =
            (Namespace*)SReadRef(ctx, buf, kSerialKindNamespace);
        return;
      }
      if (field == 25) {
        r->value->parent_class =
            (Struct*)SReadRef(ctx, buf, kSerialKindStruct);
        return;
      }
      if (field == 26) {
        uint64_t base_index = 0;
        WireReadUint64(buf, &base_index);
        r->value->base_index = (size_t)base_index;
        return;
      }
      if (field == 27) {
        uint64_t location = 0;
        WireReadUint64(buf, &location);
        r->value->location = (SourceLocation)location;
        return;
      }
      break;
    }
    case kASTShapeSplice: {
      SpliceASTNode* s = (SpliceASTNode*)n;
      if (field == 16) {
        s->reflection = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        int32_t context = 0;
        WireReadInt32(buf, &context);
        s->context = (SpliceContext)context;
        return;
      }
      break;
    }
    case kASTShapeMacro: {
      MacroNameASTNode* m = (MacroNameASTNode*)n;
      if (field == 16) {
        SReadStringVal(ctx, buf, &m->macro_name);
        return;
      }
      break;
    }
    case kASTShapeExprStmt: {
      ExpressionStatementASTNode* e = (ExpressionStatementASTNode*)n;
      if (field == 16) {
        e->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeStaticAssert: {
      StaticAssertASTNode* a = (StaticAssertASTNode*)n;
      if (field == 16) {
        a->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        SReadStringVal(ctx, buf, &a->message);
        return;
      }
      if (field == 18) {
        a->message_expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeContractAssert: {
      ContractAssertASTNode* a = (ContractAssertASTNode*)n;
      if (field == 16) {
        a->predicate = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        SerialReadAttributeVector(ctx, buf, &a->attributes);
        return;
      }
      break;
    }
    case kASTShapeIf: {
      IfStatementASTNode* i = (IfStatementASTNode*)n;
      if (field == 16) {
        i->cond = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        i->if_part = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        i->else_part = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 19) {
        WireReadBool(buf, &i->is_constexpr);
        return;
      }
      if (field == 20) {
        WireReadBool(buf, &i->is_consteval);
        return;
      }
      if (field == 21) {
        WireReadBool(buf, &i->consteval_negated);
        return;
      }
      break;
    }
    case kASTShapeCombined: {
      CombinedStatementASTNode* c = (CombinedStatementASTNode*)n;
      if (field == 16) {
        c->cond = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        c->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeThrow: {
      ThrowASTNode* t = (ThrowASTNode*)n;
      if (field == 16) {
        t->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeCompound: {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)n;
      if (field == 16) {
        c->statements = ReadASTVectorPtr(ctx, buf);
        return;
      }
      if (field == 17) {
        c->low_pc = (struct LabelASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        c->high_pc = (struct LabelASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeCatch: {
      CatchASTNode* c = (CatchASTNode*)n;
      if (field == 16) {
        c->symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 17) {
        c->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        WireReadBool(buf, &c->is_catch_all);
        return;
      }
      break;
    }
    case kASTShapeTry: {
      TryASTNode* t = (TryASTNode*)n;
      if (field == 16) {
        t->try_stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        t->catches = ReadASTVectorPtr(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeFor: {
      ForStatementASTNode* f = (ForStatementASTNode*)n;
      if (field == 16) {
        f->c1 = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        f->c2 = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        f->c3 = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 19) {
        f->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeExpansionFor: {
      ExpansionStatementASTNode* e = (ExpansionStatementASTNode*)n;
      if (field == 16) {
        e->init_stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        int32_t kind = 0;
        WireReadInt32(buf, &kind);
        e->item_kind = (ExpansionItemKind)kind;
        return;
      }
      if (field == 18) {
        e->item_symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 19) {
        e->binding_type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        return;
      }
      if (field == 20) {
        e->binding_names = NewVector();
        SReadStringVector(ctx, buf, e->binding_names);
        return;
      }
      if (field == 21) {
        e->binding_symbols = NewVector();
        SReadRefVector(ctx, buf, kSerialKindSymbol, e->binding_symbols);
        return;
      }
      if (field == 22) {
        int32_t pack_index = -1;
        WireReadInt32(buf, &pack_index);
        e->binding_pack_index = pack_index;
        return;
      }
      if (field == 23) {
        int32_t kind = 0;
        WireReadInt32(buf, &kind);
        e->init_kind = (ExpansionInitializerKind)kind;
        return;
      }
      if (field == 24) {
        e->initializer = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 25) {
        e->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeVarDecl: {
      VariableDeclarationASTNode* v = (VariableDeclarationASTNode*)n;
      if (field == 16) {
        v->symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 17) {
        v->initializer = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        v->local_static_guard =
            (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        return;
      }
      if (field == 19) {
        int64_t kind = 0;
        WireReadInt64(buf, &kind);
        v->local_static_init_kind = (LocalStaticInitKind)kind;
        return;
      }
      break;
    }
    case kASTShapeDeclList: {
      DeclarationListASTNode* d = (DeclarationListASTNode*)n;
      if (field == 16) {
        d->declarations = ReadASTVectorPtr(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeCaseLabel: {
      CaseLabelASTNode* c = (CaseLabelASTNode*)n;
      if (field == 16) {
        c->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        c->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        WireReadInt64(buf, &c->value);
        return;
      }
      break;
    }
    case kASTShapeSwitch: {
      SwitchStatementASTNode* s = (SwitchStatementASTNode*)n;
      if (field == 16) {
        s->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        s->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        SReadRefVector(ctx, buf, kSerialKindAST, &s->cases);
        return;
      }
      if (field == 19) {
        s->default_node =
            (CaseLabelASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeLabel: {
      LabelASTNode* l = (LabelASTNode*)n;
      if (field == 16) {
        l->stmt = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        SReadStringVal(ctx, buf, &l->name);
        return;
      }
      if (field == 18) {
        WireReadBool(buf, &l->named);
        return;
      }
      break;
    }
    case kASTShapeAsm: {
      AsmASTNode* a = (AsmASTNode*)n;
      if (field == 16) {
        a->text = SReadStringPtr(ctx, buf);
        return;
      }
      if (field == 17) {
        WireReadBool(buf, &a->is_volatile);
        return;
      }
      if (field == 18) {
        WireReadBool(buf, &a->is_goto);
        return;
      }
      if (field == 19) {
        ReadAsmOperandVector(ctx, buf, &a->outputs);
        return;
      }
      if (field == 20) {
        ReadAsmOperandVector(ctx, buf, &a->inputs);
        return;
      }
      if (field == 21) {
        SReadStringVector(ctx, buf, &a->clobbers);
        return;
      }
      if (field == 22) {
        SReadStringVector(ctx, buf, &a->labels);
        return;
      }
      if (field == 23) {
        SReadRefVector(ctx, buf, kSerialKindAST, &a->label_nodes);
        return;
      }
      break;
    }
    case kASTShapeGoto: {
      GotoStatementASTNode* g = (GotoStatementASTNode*)n;
      if (field == 16) {
        g->label_name = SReadStringPtr(ctx, buf);
        return;
      }
      if (field == 17) {
        g->label = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 18) {
        g->lca = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapePtrScale: {
      PtrScaleASTNode* p = (PtrScaleASTNode*)n;
      if (field == 16) {
        p->ref_type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        TypeRecordIncRef(p->ref_type);
        return;
      }
      if (field == 17) {
        int32_t v;
        WireReadInt32(buf, &v);
        p->scale_op = (ASTOpcode)v;
        return;
      }
      if (field == 18) {
        p->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeExprInit: {
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)n;
      if (field == 16) {
        e->expr = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeBracedInit: {
      BracedInitializerASTNode* b = (BracedInitializerASTNode*)n;
      if (field == 16) {
        b->initializers = ReadASTVectorPtr(ctx, buf);
        return;
      }
      break;
    }
    case kASTShapeDesignatedInit: {
      DesignatedInitializerASTNode* d = (DesignatedInitializerASTNode*)n;
      if (field == 16) {
        d->designators = ReadDesignatorVector(ctx, buf);
        return;
      }
      if (field == 17) {
        d->init = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeCompoundLiteral: {
      CompoundLiteralASTNode* c = (CompoundLiteralASTNode*)n;
      if (field == 16) {
        c->sym = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      if (field == 17) {
        c->initializer = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        return;
      }
      break;
    }
    case kASTShapeBase:
    default:
      break;
  }
  WireSkip(buf, wt);
}

// ---------------------------------------------------------------------------
// Pooled AST kind: write / alloc / read.
// ---------------------------------------------------------------------------
static bool WriteAST(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  ASTNode* n = (ASTNode*)obj;
  ASTNodeShape shape = ASTNodeGetShape(n);
  WriteASTBase(ctx, buf, n, shape);
  WriteASTSub(ctx, buf, n, shape);
  return !WireBufferHasError(buf);
}

static bool CanInternAST(const void* obj) {
  const ASTNode* n = (const ASTNode*)obj;
  // ASTNodeDelete releases shape-specific heap storage while leaving the
  // arena-backed node address intact.  References that outlive that deletion
  // are stale graph edges and cannot be serialized safely.
  return (n->flags & kASTDestructed) == 0;
}

// Scans the record for the opcode (field 1) and shape (field 2) so the correct
// concrete node can be allocated.
static void* AllocAST(DeserializeContext* ctx, const void* blob, size_t len) {
  (void)ctx;
  WireBuffer in;
  WireBufferInitReader(&in, blob, len);
  int op = 0;
  int shape = kASTShapeBase;
  bool have_op = false;
  bool have_shape = false;
  while (!WireBufferEof(&in) && !WireBufferHasError(&in) &&
         (!have_op || !have_shape)) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    if (field == kAST_op) {
      WireReadInt32(&in, &op);
      have_op = true;
    } else if (field == kAST_shape) {
      WireReadInt32(&in, &shape);
      have_shape = true;
    } else {
      WireSkip(&in, wt);
    }
  }
  return ASTNodeAllocForShape((ASTNodeShape)shape, (ASTOpcode)op);
}

static bool ReadAST(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  ASTNode* n = (ASTNode*)obj;
  ASTNodeShape shape = ASTNodeGetShape(n);
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    if (ReadASTBaseField(ctx, buf, n, field)) {
      continue;
    }
    ReadASTSubField(ctx, buf, n, shape, field, wt);
  }
  return !WireBufferHasError(buf);
}

void SerializeRegisterASTKinds(void) {
  static const SerialKindVtable ast_vt = {
      WriteAST, AllocAST, ReadAST, "AST", CanInternAST};
  SerializeRegisterKind(kSerialKindAST, &ast_vt);
  SerializeRegisterFields(kSerialKindAST, kASTBaseFields,
                          sizeof(kASTBaseFields) / sizeof(kASTBaseFields[0]));
}
