//
//  symbol_serialize.c
//  c_compiler
//
//  Serialization of Symbol and Namespace records (plus their inline sub-objects
//  Attribute and VariableTemplate).
//
//  Notes / intentional omissions:
//   * Symbol.value is a discriminated union with no stored discriminant; only
//     the scalar payload (value.ivalue bits, which also covers fvalue and
//     arg_number) is round-tripped.  Pointer members of the union (func_defn,
//     other) are codegen/transient and are not serialized.
//   * usage_info and die are codegen/debug-only and are omitted.
//   * Namespace symbol/tag tables are stored as flat symbol handle lists and
//     rebuilt via NamespaceInsertSymbol/Tag on load.
//

#include <stdlib.h>

#include "ast.h"
#include "concepts.h"
#include "constraint_serialize.h"
#include "serialize_common.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"

//
// Symbol field numbers.
//
enum {
  kSym_name = 1,
  kSym_asm_name = 2,
  kSym_namespace = 3,
  kSym_id = 4,
  kSym_type = 5,
  kSym_storage = 6,
  // Flags: 7..31.
  kSym_is_defined = 7,
  kSym_is_tentative_decl = 8,
  kSym_is_forward_declared = 9,
  kSym_is_local = 10,
  kSym_is_block_scope = 11,
  kSym_is_argument = 12,
  kSym_is_temp = 13,
  kSym_address_taken = 14,
  kSym_used = 15,
  kSym_invented = 16,
  kSym_is_inline_defn = 17,
  kSym_value_set = 18,
  kSym_noreturn = 19,
  kSym_always_inline = 20,
  kSym_noinline = 21,
  kSym_is_using_alias = 22,
  kSym_is_overloaded = 23,
  kSym_is_template = 24,
  kSym_is_template_parameter = 25,
  kSym_is_template_type_parameter = 26,
  kSym_is_parameter_pack = 27,
  kSym_is_constexpr = 28,
  kSym_is_constinit = 29,
  kSym_is_weak = 30,
  kSym_is_c_linkage = 31,
  // Non-flag members: 32+.
  kSym_alignment = 32,
  kSym_template_parameter_index = 33,
  kSym_dependent_value_template_parameter_index = 34,
  kSym_location = 35,
  kSym_value_ivalue = 36,
  kSym_stack_offset = 37,
  kSym_alias_target = 38,
  kSym_overload_next = 39,
  kSym_default_argument = 40,
  kSym_attributes = 42,
  kSym_is_exported = 41,
  kSym_is_concept = 43,
  kSym_variable_template = 44,
  kSym_concept_definition = 45,
};

static const WireFieldDesc kSymbolFields[] = {
    {kSym_name, "name"},
    {kSym_asm_name, "asm_name"},
    {kSym_namespace, "namespace_"},
    {kSym_id, "id"},
    {kSym_type, "type"},
    {kSym_storage, "storage"},
    {kSym_is_defined, "is_defined"},
    {kSym_is_tentative_decl, "is_tentative_decl"},
    {kSym_is_forward_declared, "is_forward_declared"},
    {kSym_is_local, "is_local"},
    {kSym_is_block_scope, "is_block_scope"},
    {kSym_is_argument, "is_argument"},
    {kSym_is_temp, "is_temp"},
    {kSym_address_taken, "address_taken"},
    {kSym_used, "used"},
    {kSym_invented, "invented"},
    {kSym_is_inline_defn, "is_inline_defn"},
    {kSym_value_set, "value_set"},
    {kSym_noreturn, "noreturn"},
    {kSym_always_inline, "always_inline"},
    {kSym_noinline, "noinline"},
    {kSym_is_using_alias, "is_using_alias"},
    {kSym_is_overloaded, "is_overloaded"},
    {kSym_is_template, "is_template"},
    {kSym_is_template_parameter, "is_template_parameter"},
    {kSym_is_template_type_parameter, "is_template_type_parameter"},
    {kSym_is_parameter_pack, "is_parameter_pack"},
    {kSym_is_constexpr, "is_constexpr"},
    {kSym_is_constinit, "is_constinit"},
    {kSym_is_weak, "is_weak"},
    {kSym_is_c_linkage, "is_c_linkage"},
    {kSym_is_exported, "is_exported"},
    {kSym_alignment, "alignment"},
    {kSym_template_parameter_index, "template_parameter_index"},
    {kSym_dependent_value_template_parameter_index,
     "dependent_value_template_parameter_index"},
    {kSym_location, "location"},
    {kSym_value_ivalue, "value.ivalue"},
    {kSym_stack_offset, "stack_offset"},
    {kSym_alias_target, "alias_target"},
    {kSym_overload_next, "overload_next"},
    {kSym_default_argument, "default_argument"},
    {kSym_attributes, "attributes"},
    {kSym_is_concept, "is_concept"},
    {kSym_variable_template, "variable_template"},
    {kSym_concept_definition, "concept_definition"},
};

//
// Attribute (inline sub-message) field numbers.
//
enum {
  kAttr_name = 1,
  kAttr_args = 2,
};

//
// Namespace field numbers.
//
enum {
  kNs_name = 1,
  kNs_qualified_name = 2,
  kNs_is_anonymous = 3,
  kNs_children = 4,
  kNs_parent = 5,
  kNs_anonymous_child = 6,
  kNs_symbols = 7,
  kNs_tags = 8,
  kNs_is_inline = 9,
  kNs_alias_names = 10,
  kNs_alias_targets = 11,
};

static const WireFieldDesc kNamespaceFields[] = {
    {kNs_name, "name"},
    {kNs_qualified_name, "qualified_name"},
    {kNs_is_anonymous, "is_anonymous"},
    {kNs_children, "children"},
    {kNs_parent, "parent"},
    {kNs_anonymous_child, "anonymous_child"},
    {kNs_symbols, "symbols"},
    {kNs_tags, "tags"},
    {kNs_is_inline, "is_inline"},
    {kNs_alias_names, "alias_names"},
    {kNs_alias_targets, "alias_targets"},
};

// ---------------------------------------------------------------------------
// Attribute vector (inline).
// ---------------------------------------------------------------------------
static void WriteAttributeVector(SerializeContext* ctx, WireBuffer* buf,
                                 int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  WireWriteRawVarint(&tmp, (uint64_t)v->length);
  for (size_t i = 0; i < v->length; i++) {
    Attribute* a = (Attribute*)VectorGet(v, i);
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    SWriteStringVal(ctx, &elem, kAttr_name, &a->name);
    SWriteStringVector(ctx, &elem, kAttr_args, &a->args);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadAttributeVector(DeserializeContext* ctx, WireBuffer* in,
                                Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
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
    Attribute* a = NewAttribute("");
    while (!WireBufferEof(&er) && !WireBufferHasError(&er)) {
      int field;
      WireType wt;
      if (!WireReadTag(&er, &field, &wt)) {
        break;
      }
      switch (field) {
        case kAttr_name:
          SReadStringVal(ctx, &er, &a->name);
          break;
        case kAttr_args:
          SReadStringVector(ctx, &er, &a->args);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    VectorAppend(out, a);
  }
}

// ---------------------------------------------------------------------------
// VariableTemplate (inline sub-message).
// ---------------------------------------------------------------------------
enum {
  kVt_initializer = 1,  // ASTNode ref (unanalyzed initializer expression).
  kVt_parameters = 2,   // TemplateParameter vector.
};

static void WriteVariableTemplate(SerializeContext* ctx, WireBuffer* buf,
                                  int field, VariableTemplate* vt) {
  WireBuffer sub;
  WireBufferInitOwned(&sub, 32);
  SWriteRef(ctx, &sub, kVt_initializer, kSerialKindAST, vt->initializer);
  SerialWriteTemplateParameterVector(ctx, &sub, kVt_parameters,
                                     &vt->parameters);
  WireWriteBytes(buf, field, WireBufferData(&sub), WireBufferSize(&sub));
  WireBufferDestruct(&sub);
}

static VariableTemplate* ReadVariableTemplate(DeserializeContext* ctx,
                                              WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return NULL;
  }
  // Allocated exactly as the parser does (see syntax.c), so SymbolDestruct
  // frees it correctly.
  VariableTemplate* vt = (VariableTemplate*)malloc(sizeof(VariableTemplate));
  vt->initializer = NULL;
  VectorInit(&vt->parameters);
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  while (!WireBufferEof(&sub) && !WireBufferHasError(&sub)) {
    int field;
    WireType wt;
    if (!WireReadTag(&sub, &field, &wt)) {
      break;
    }
    switch (field) {
      case kVt_initializer:
        vt->initializer = (ASTNode*)SReadRef(ctx, &sub, kSerialKindAST);
        break;
      case kVt_parameters:
        SerialReadTemplateParameterVector(ctx, &sub, &vt->parameters);
        break;
      default:
        WireSkip(&sub, wt);
        break;
    }
  }
  return vt;
}

// ---------------------------------------------------------------------------
// Symbol.
// ---------------------------------------------------------------------------
static void WriteBoolField(WireBuffer* buf, int field, bool value) {
  WireWriteBool(buf, field, value);
}

static bool WriteSymbol(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  Symbol* s = (Symbol*)obj;
  SWriteStringVal(ctx, buf, kSym_name, &s->name);
  SWriteStringVal(ctx, buf, kSym_asm_name, &s->asm_name);
  SWriteRef(ctx, buf, kSym_namespace, kSerialKindNamespace, s->namespace_);
  WireWriteInt32(buf, kSym_id, s->id);
  SWriteRef(ctx, buf, kSym_type, kSerialKindType, s->type);
  WireWriteInt32(buf, kSym_storage, (int32_t)s->storage);

  WriteBoolField(buf, kSym_is_defined, s->flags.is_defined);
  WriteBoolField(buf, kSym_is_tentative_decl, s->flags.is_tentative_decl);
  WriteBoolField(buf, kSym_is_forward_declared, s->flags.is_forward_declared);
  WriteBoolField(buf, kSym_is_local, s->flags.is_local);
  WriteBoolField(buf, kSym_is_block_scope, s->flags.is_block_scope);
  WriteBoolField(buf, kSym_is_argument, s->flags.is_argument);
  WriteBoolField(buf, kSym_is_temp, s->flags.is_temp);
  WriteBoolField(buf, kSym_address_taken, s->flags.address_taken);
  WriteBoolField(buf, kSym_used, s->flags.used);
  WriteBoolField(buf, kSym_invented, s->flags.invented);
  WriteBoolField(buf, kSym_is_inline_defn, s->flags.is_inline_defn);
  WriteBoolField(buf, kSym_value_set, s->flags.value_set);
  WriteBoolField(buf, kSym_noreturn, s->flags.noreturn);
  WriteBoolField(buf, kSym_always_inline, s->flags.always_inline);
  WriteBoolField(buf, kSym_noinline, s->flags.noinline);
  WriteBoolField(buf, kSym_is_using_alias, s->flags.is_using_alias);
  WriteBoolField(buf, kSym_is_overloaded, s->flags.is_overloaded);
  WriteBoolField(buf, kSym_is_template, s->flags.is_template);
  WriteBoolField(buf, kSym_is_template_parameter,
                 s->flags.is_template_parameter);
  WriteBoolField(buf, kSym_is_template_type_parameter,
                 s->flags.is_template_type_parameter);
  WriteBoolField(buf, kSym_is_parameter_pack, s->flags.is_parameter_pack);
  WriteBoolField(buf, kSym_is_constexpr, s->flags.is_constexpr);
  WriteBoolField(buf, kSym_is_constinit, s->flags.is_constinit);
  WriteBoolField(buf, kSym_is_weak, s->flags.is_weak);
  WriteBoolField(buf, kSym_is_c_linkage, s->flags.is_c_linkage);
  WriteBoolField(buf, kSym_is_exported, s->flags.is_exported);
  WriteBoolField(buf, kSym_is_concept, s->flags.is_concept);

  WireWriteInt32(buf, kSym_alignment, s->alignment);
  WireWriteInt32(buf, kSym_template_parameter_index,
                 s->template_parameter_index);
  WireWriteInt32(buf, kSym_dependent_value_template_parameter_index,
                 s->dependent_value_template_parameter_index);
  WireWriteUint64(buf, kSym_location, (uint64_t)s->location);
  WireWriteInt64(buf, kSym_value_ivalue, s->value.ivalue);
  WireWriteInt32(buf, kSym_stack_offset, s->stack_offset);
  SWriteRef(ctx, buf, kSym_alias_target, kSerialKindSymbol, s->alias_target);
  SWriteRef(ctx, buf, kSym_overload_next, kSerialKindSymbol, s->overload_next);
  SWriteRef(ctx, buf, kSym_default_argument, kSerialKindAST,
            s->default_argument);
  WriteAttributeVector(ctx, buf, kSym_attributes, &s->attributes);
  if (s->variable_template != NULL) {
    WriteVariableTemplate(ctx, buf, kSym_variable_template,
                          s->variable_template);
  }
  SerialWriteConcept(ctx, buf, kSym_concept_definition, s->concept_definition);
  return !WireBufferHasError(buf);
}

static void* AllocSymbol(DeserializeContext* ctx, const void* blob,
                         size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewSymbol("", NULL, STO(implicit));
}

static bool ReadSymbol(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  Symbol* s = (Symbol*)obj;
  bool b;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kSym_name:
        SReadStringVal(ctx, buf, &s->name);
        break;
      case kSym_asm_name:
        SReadStringVal(ctx, buf, &s->asm_name);
        break;
      case kSym_namespace:
        s->namespace_ = (Namespace*)SReadRef(ctx, buf, kSerialKindNamespace);
        break;
      case kSym_id:
        WireReadInt32(buf, &s->id);
        break;
      case kSym_type:
        s->type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        break;
      case kSym_storage: {
        int32_t v;
        WireReadInt32(buf, &v);
        s->storage = (Storage)v;
        break;
      }
      case kSym_is_defined:
        WireReadBool(buf, &b);
        s->flags.is_defined = b;
        break;
      case kSym_is_tentative_decl:
        WireReadBool(buf, &b);
        s->flags.is_tentative_decl = b;
        break;
      case kSym_is_forward_declared:
        WireReadBool(buf, &b);
        s->flags.is_forward_declared = b;
        break;
      case kSym_is_local:
        WireReadBool(buf, &b);
        s->flags.is_local = b;
        break;
      case kSym_is_block_scope:
        WireReadBool(buf, &b);
        s->flags.is_block_scope = b;
        break;
      case kSym_is_argument:
        WireReadBool(buf, &b);
        s->flags.is_argument = b;
        break;
      case kSym_is_temp:
        WireReadBool(buf, &b);
        s->flags.is_temp = b;
        break;
      case kSym_address_taken:
        WireReadBool(buf, &b);
        s->flags.address_taken = b;
        break;
      case kSym_used:
        WireReadBool(buf, &b);
        s->flags.used = b;
        break;
      case kSym_invented:
        WireReadBool(buf, &b);
        s->flags.invented = b;
        break;
      case kSym_is_inline_defn:
        WireReadBool(buf, &b);
        s->flags.is_inline_defn = b;
        break;
      case kSym_value_set:
        WireReadBool(buf, &b);
        s->flags.value_set = b;
        break;
      case kSym_noreturn:
        WireReadBool(buf, &b);
        s->flags.noreturn = b;
        break;
      case kSym_always_inline:
        WireReadBool(buf, &b);
        s->flags.always_inline = b;
        break;
      case kSym_noinline:
        WireReadBool(buf, &b);
        s->flags.noinline = b;
        break;
      case kSym_is_using_alias:
        WireReadBool(buf, &b);
        s->flags.is_using_alias = b;
        break;
      case kSym_is_overloaded:
        WireReadBool(buf, &b);
        s->flags.is_overloaded = b;
        break;
      case kSym_is_template:
        WireReadBool(buf, &b);
        s->flags.is_template = b;
        break;
      case kSym_is_template_parameter:
        WireReadBool(buf, &b);
        s->flags.is_template_parameter = b;
        break;
      case kSym_is_template_type_parameter:
        WireReadBool(buf, &b);
        s->flags.is_template_type_parameter = b;
        break;
      case kSym_is_parameter_pack:
        WireReadBool(buf, &b);
        s->flags.is_parameter_pack = b;
        break;
      case kSym_is_constexpr:
        WireReadBool(buf, &b);
        s->flags.is_constexpr = b;
        break;
      case kSym_is_constinit:
        WireReadBool(buf, &b);
        s->flags.is_constinit = b;
        break;
      case kSym_is_weak:
        WireReadBool(buf, &b);
        s->flags.is_weak = b;
        break;
      case kSym_is_c_linkage:
        WireReadBool(buf, &b);
        s->flags.is_c_linkage = b;
        break;
      case kSym_is_exported:
        WireReadBool(buf, &b);
        s->flags.is_exported = b;
        break;
      case kSym_is_concept:
        WireReadBool(buf, &b);
        s->flags.is_concept = b;
        break;
      case kSym_alignment:
        WireReadInt32(buf, &s->alignment);
        break;
      case kSym_template_parameter_index:
        WireReadInt32(buf, &s->template_parameter_index);
        break;
      case kSym_dependent_value_template_parameter_index:
        WireReadInt32(buf, &s->dependent_value_template_parameter_index);
        break;
      case kSym_location: {
        uint64_t v;
        WireReadUint64(buf, &v);
        s->location = (SourceLocation)v;
        break;
      }
      case kSym_value_ivalue:
        WireReadInt64(buf, &s->value.ivalue);
        break;
      case kSym_stack_offset:
        WireReadInt32(buf, &s->stack_offset);
        break;
      case kSym_alias_target:
        s->alias_target = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kSym_overload_next:
        s->overload_next = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kSym_default_argument:
        s->default_argument = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kSym_attributes:
        ReadAttributeVector(ctx, buf, &s->attributes);
        break;
      case kSym_variable_template:
        s->variable_template = ReadVariableTemplate(ctx, buf);
        break;
      case kSym_concept_definition:
        s->concept_definition = SerialReadConcept(ctx, buf);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

// ---------------------------------------------------------------------------
// Namespace.
// ---------------------------------------------------------------------------
static void CollectSymbol(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  SymbolNode* sn = (SymbolNode*)node;
  VectorAppend((Vector*)data, sn->symbol);
}

static bool WriteNamespace(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  Namespace* ns = (Namespace*)obj;
  SWriteStringVal(ctx, buf, kNs_name, &ns->name);
  SWriteStringVal(ctx, buf, kNs_qualified_name, &ns->qualified_name);
  WireWriteBool(buf, kNs_is_anonymous, ns->is_anonymous);
  WireWriteBool(buf, kNs_is_inline, ns->is_inline);
  SWriteRefVector(ctx, buf, kNs_children, kSerialKindNamespace, &ns->children);
  SWriteRef(ctx, buf, kNs_parent, kSerialKindNamespace, ns->parent);
  SWriteRef(ctx, buf, kNs_anonymous_child, kSerialKindNamespace,
            ns->anonymous_child);

  Vector alias_names;
  Vector alias_targets;
  VectorInit(&alias_names);
  VectorInit(&alias_targets);
  for (size_t i = 0; i < ns->namespace_aliases.length; i++) {
    NamespaceAlias* alias =
        (NamespaceAlias*)VectorGet(&ns->namespace_aliases, i);
    if (alias != NULL) {
      VectorAppend(&alias_names, &alias->name);
      VectorAppend(&alias_targets, alias->target);
    }
  }
  SWriteStringVector(ctx, buf, kNs_alias_names, &alias_names);
  SWriteRefVector(ctx, buf, kNs_alias_targets, kSerialKindNamespace,
                  &alias_targets);
  VectorDestruct(&alias_names);
  VectorDestruct(&alias_targets);

  Vector symbols;
  VectorInit(&symbols);
  BinaryTreeTraverse(&ns->symbol_table, CollectSymbol, &symbols);
  SWriteRefVector(ctx, buf, kNs_symbols, kSerialKindSymbol, &symbols);
  VectorDestruct(&symbols);

  Vector tags;
  VectorInit(&tags);
  BinaryTreeTraverse(&ns->tag_table, CollectSymbol, &tags);
  SWriteRefVector(ctx, buf, kNs_tags, kSerialKindSymbol, &tags);
  VectorDestruct(&tags);
  return !WireBufferHasError(buf);
}

static void* AllocNamespace(DeserializeContext* ctx, const void* blob,
                            size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewNamespace("", NULL, false);
}

static bool ReadNamespace(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  Namespace* ns = (Namespace*)obj;
  Vector alias_names;
  Vector alias_targets;
  VectorInit(&alias_names);
  VectorInit(&alias_targets);
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kNs_name:
        SReadStringVal(ctx, buf, &ns->name);
        break;
      case kNs_qualified_name:
        SReadStringVal(ctx, buf, &ns->qualified_name);
        break;
      case kNs_is_anonymous:
        WireReadBool(buf, &ns->is_anonymous);
        break;
      case kNs_is_inline:
        WireReadBool(buf, &ns->is_inline);
        break;
      case kNs_children:
        SReadRefVector(ctx, buf, kSerialKindNamespace, &ns->children);
        break;
      case kNs_parent:
        ns->parent = (Namespace*)SReadRef(ctx, buf, kSerialKindNamespace);
        break;
      case kNs_anonymous_child:
        ns->anonymous_child =
            (Namespace*)SReadRef(ctx, buf, kSerialKindNamespace);
        break;
      case kNs_alias_names:
        SReadStringVector(ctx, buf, &alias_names);
        break;
      case kNs_alias_targets:
        SReadRefVector(ctx, buf, kSerialKindNamespace, &alias_targets);
        break;
      case kNs_symbols: {
        Vector symbols;
        VectorInit(&symbols);
        SReadRefVector(ctx, buf, kSerialKindSymbol, &symbols);
        for (size_t i = 0; i < symbols.length; i++) {
          Symbol* sym = (Symbol*)VectorGet(&symbols, i);
          if (sym != NULL) {
            NamespaceInsertSymbol(ns, sym);
          }
        }
        VectorDestruct(&symbols);
        break;
      }
      case kNs_tags: {
        Vector tags;
        VectorInit(&tags);
        SReadRefVector(ctx, buf, kSerialKindSymbol, &tags);
        for (size_t i = 0; i < tags.length; i++) {
          Symbol* sym = (Symbol*)VectorGet(&tags, i);
          if (sym != NULL) {
            NamespaceInsertTag(ns, sym);
          }
        }
        VectorDestruct(&tags);
        break;
      }
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  size_t alias_count = alias_names.length < alias_targets.length
      ? alias_names.length
      : alias_targets.length;
  for (size_t i = 0; i < alias_count; i++) {
    String* name = (String*)VectorGet(&alias_names, i);
    Namespace* target = (Namespace*)VectorGet(&alias_targets, i);
    if (name != NULL && target != NULL) {
      NamespaceInsertAlias(ns, name, target);
    }
  }
  for (size_t i = 0; i < alias_names.length; i++) {
    StringDelete((String*)VectorGet(&alias_names, i));
  }
  VectorDestruct(&alias_names);
  VectorDestruct(&alias_targets);
  return !WireBufferHasError(buf);
}

void SerializeRegisterSymbolKinds(void) {
  static const SerialKindVtable symbol_vt = {WriteSymbol, AllocSymbol,
                                             ReadSymbol, "Symbol"};
  static const SerialKindVtable namespace_vt = {WriteNamespace, AllocNamespace,
                                               ReadNamespace, "Namespace"};
  SerializeRegisterKind(kSerialKindSymbol, &symbol_vt);
  SerializeRegisterKind(kSerialKindNamespace, &namespace_vt);
  SerializeRegisterFields(kSerialKindSymbol, kSymbolFields,
                          sizeof(kSymbolFields) / sizeof(kSymbolFields[0]));
  SerializeRegisterFields(kSerialKindNamespace, kNamespaceFields,
                          sizeof(kNamespaceFields) /
                              sizeof(kNamespaceFields[0]));
}
