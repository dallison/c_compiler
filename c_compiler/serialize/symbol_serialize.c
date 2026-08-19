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
#include "constexpr.h"
#include "reflection.h"
#include "serialize_common.h"
#include "symbol.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type.h"
#include "type_class_internal.h"

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
  kSym_associated_constraint = 46,
  kSym_alias_template = 47,
  kSym_cxx_linkage = 48,
  kSym_owning_module_name = 49,
  kSym_owning_module_partition = 50,
  kSym_import_source_module = 51,
  kSym_is_module_private = 52,
  kSym_func_defn = 53,
  kSym_is_explicit_specialization = 54,
  kSym_is_template_template_parameter = 55,
  kSym_template_template_parameters = 56,
  kSym_template_constructor_initializers = 57,
  kSym_constexpr_initializer = 58,
  kSym_is_name_independent = 59,
  kSym_name_independent_lookup_ambiguous = 60,
  kSym_template_template_parameter_kind = 61,
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
    {kSym_associated_constraint, "associated_constraint"},
    {kSym_alias_template, "alias_template"},
    {kSym_cxx_linkage, "cxx_linkage"},
    {kSym_owning_module_name, "owning_module_name"},
    {kSym_owning_module_partition, "owning_module_partition"},
    {kSym_import_source_module, "import_source_module"},
    {kSym_is_module_private, "is_module_private"},
    {kSym_func_defn, "func_defn"},
    {kSym_is_explicit_specialization, "is_explicit_specialization"},
    {kSym_is_template_template_parameter,
     "is_template_template_parameter"},
    {kSym_template_template_parameters, "template_template_parameters"},
    {kSym_template_constructor_initializers,
     "template_constructor_initializers"},
    {kSym_constexpr_initializer, "constexpr_initializer"},
    {kSym_is_name_independent, "is_name_independent"},
    {kSym_name_independent_lookup_ambiguous,
     "name_independent_lookup_ambiguous"},
    {kSym_template_template_parameter_kind,
     "template_template_parameter_kind"},
};

//
// Attribute (inline sub-message) field numbers.
//
enum {
  kAttr_name = 1,
  kAttr_args = 2,
  kAttr_dependent_alignas_type = 3,
  kAttr_dependent_alignas_expr = 4,
  kAttr_annotation_expr = 5,
  kAttr_namespace = 7,
  kAttr_token = 8,
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
void SerialWriteAttributeVector(SerializeContext* ctx, WireBuffer* buf,
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
    SWriteRef(ctx, &elem, kAttr_dependent_alignas_type,
              kSerialKindType, a->dependent_alignas_type);
    SWriteRef(ctx, &elem, kAttr_dependent_alignas_expr,
              kSerialKindAST, a->dependent_alignas_expr);
    SWriteRef(ctx, &elem, kAttr_annotation_expr,
              kSerialKindAST, a->annotation_expr);
    SWriteStringVal(ctx, &elem, kAttr_namespace, &a->attribute_namespace);
    SWriteStringVal(ctx, &elem, kAttr_token, &a->token);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

void SerialReadAttributeVector(DeserializeContext* ctx, WireBuffer* in,
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
        case kAttr_dependent_alignas_type:
          a->dependent_alignas_type =
              (TypeRecord*)SReadRef(ctx, &er, kSerialKindType);
          break;
        case kAttr_dependent_alignas_expr:
          a->dependent_alignas_expr =
              (ASTNode*)SReadRef(ctx, &er, kSerialKindAST);
          break;
        case kAttr_annotation_expr:
          a->annotation_expr =
              (ASTNode*)SReadRef(ctx, &er, kSerialKindAST);
          break;
        case kAttr_namespace:
          SReadStringVal(ctx, &er, &a->attribute_namespace);
          break;
        case kAttr_token:
          SReadStringVal(ctx, &er, &a->token);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    // Modules written before P3385 only contain the normalized semantic name.
    // Use it as the best available source token for compatibility.
    if (a->token.length == 0 && a->name.length > 0) {
      StringSetString(&a->token, &a->name);
    }
    VectorAppend(out, a);
  }
}

enum {
  kCtorInit_name = 1,
  kCtorInit_actuals = 2,
  kCtorInit_location = 3,
};

static void WriteTemplateConstructorInitializers(SerializeContext* ctx,
                                                 WireBuffer* out, int field,
                                                 Symbol* symbol) {
  CXXConstructorInitList* initializers =
      FindTemplateConstructorInitializers(symbol);
  if (initializers == NULL || initializers->deferred_initializers.length == 0) {
    return;
  }
  WireBuffer list;
  WireBufferInitOwned(&list, 32);
  WireWriteRawVarint(
      &list, (uint64_t)initializers->deferred_initializers.length);
  for (size_t i = 0; i < initializers->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* initializer =
        (CXXDeferredConstructorInitializer*)VectorGet(
            &initializers->deferred_initializers, i);
    WireBuffer item;
    WireBufferInitOwned(&item, 32);
    SWriteStringVal(ctx, &item, kCtorInit_name, &initializer->name);
    if (initializer->actuals != NULL) {
      SWriteRefVector(ctx, &item, kCtorInit_actuals, kSerialKindAST,
                      initializer->actuals);
    }
    WireWriteUint64(&item, kCtorInit_location,
                    (uint64_t)initializer->location);
    WireWriteRawVarint(&list, (uint64_t)WireBufferSize(&item));
    WireWriteRaw(&list, WireBufferData(&item), WireBufferSize(&item));
    WireBufferDestruct(&item);
  }
  WireWriteBytes(out, field, WireBufferData(&list), WireBufferSize(&list));
  WireBufferDestruct(&list);
}

static void ReadTemplateConstructorInitializers(DeserializeContext* ctx,
                                                WireBuffer* in,
                                                Symbol* symbol) {
  const void* data;
  size_t length;
  if (!WireReadBytes(in, &data, &length)) {
    return;
  }
  WireBuffer list;
  WireBufferInitReader(&list, data, length);
  uint64_t count;
  if (!WireReadRawVarint(&list, &count)) {
    return;
  }
  CXXConstructorInitList* initializers =
      (CXXConstructorInitList*)malloc(sizeof(CXXConstructorInitList));
  SyntaxCXXConstructorInitListInit(initializers);
  for (uint64_t i = 0; i < count; i++) {
    const void* item_data;
    size_t item_length;
    if (!WireReadBytes(&list, &item_data, &item_length)) {
      break;
    }
    CXXDeferredConstructorInitializer* initializer =
        (CXXDeferredConstructorInitializer*)calloc(1, sizeof(*initializer));
    StringInit(&initializer->name, NULL);
    initializer->actuals = NewVector();
    WireBuffer item;
    WireBufferInitReader(&item, item_data, item_length);
    while (!WireBufferEof(&item) && !WireBufferHasError(&item)) {
      int item_field;
      WireType wire_type;
      if (!WireReadTag(&item, &item_field, &wire_type)) {
        break;
      }
      switch (item_field) {
        case kCtorInit_name:
          SReadStringVal(ctx, &item, &initializer->name);
          break;
        case kCtorInit_actuals:
          SReadRefVector(ctx, &item, kSerialKindAST, initializer->actuals);
          break;
        case kCtorInit_location: {
          uint64_t location;
          WireReadUint64(&item, &location);
          initializer->location = (SourceLocation)location;
          break;
        }
        default:
          WireSkip(&item, wire_type);
          break;
      }
    }
    VectorAppend(&initializers->deferred_initializers, initializer);
  }
  RegisterTemplateConstructorInitializers(symbol, initializers);
}

// ---------------------------------------------------------------------------
// VariableTemplate (inline sub-message).
// ---------------------------------------------------------------------------
enum {
  kVt_initializer = 1,  // ASTNode ref (unanalyzed initializer expression).
  kVt_parameters = 2,   // TemplateParameter vector.
  kVt_associated_constraint = 3,
  kVt_partial_specializations = 4,
};

static void WriteVariableTemplate(SerializeContext* ctx, WireBuffer* buf,
                                  int field, VariableTemplate* vt) {
  WireBuffer sub;
  WireBufferInitOwned(&sub, 32);
  SWriteRef(ctx, &sub, kVt_initializer, kSerialKindAST, vt->initializer);
  SerialWriteTemplateParameterVector(ctx, &sub, kVt_parameters,
                                     &vt->parameters);
  SerialWriteConstraint(ctx, &sub, kVt_associated_constraint,
                        vt->associated_constraint);
  SerialWritePartialSpecializationVector(
      ctx, &sub, kVt_partial_specializations,
      &vt->partial_specializations);
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
  vt->associated_constraint = NULL;
  VectorInit(&vt->parameters);
  VectorInit(&vt->partial_specializations);
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
      case kVt_associated_constraint:
        vt->associated_constraint = SerialReadConstraint(ctx, &sub);
        break;
      case kVt_partial_specializations:
        SerialReadPartialSpecializationVector(
            ctx, &sub, &vt->partial_specializations);
        break;
      default:
        WireSkip(&sub, wt);
        break;
    }
  }
  return vt;
}

// ---------------------------------------------------------------------------
// AliasTemplate (inline sub-message).
// ---------------------------------------------------------------------------
enum {
  kAt_parameters = 1,
  kAt_ctad_names_template_template_parameter = 2,
};

static void WriteAliasTemplate(SerializeContext* ctx, WireBuffer* buf,
                               int field, AliasTemplate* at) {
  WireBuffer sub;
  WireBufferInitOwned(&sub, 32);
  SerialWriteTemplateParameterVector(ctx, &sub, kAt_parameters,
                                     &at->parameters);
  if (at->ctad_names_template_template_parameter) {
    WireWriteBool(&sub, kAt_ctad_names_template_template_parameter, true);
  }
  WireWriteBytes(buf, field, WireBufferData(&sub), WireBufferSize(&sub));
  WireBufferDestruct(&sub);
}

static AliasTemplate* ReadAliasTemplate(DeserializeContext* ctx,
                                        WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return NULL;
  }
  AliasTemplate* at = malloc(sizeof(AliasTemplate));
  VectorInit(&at->parameters);
  at->ctad_names_template_template_parameter = false;
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  while (!WireBufferEof(&sub) && !WireBufferHasError(&sub)) {
    int field;
    WireType wt;
    if (!WireReadTag(&sub, &field, &wt)) {
      break;
    }
    if (field == kAt_parameters) {
      SerialReadTemplateParameterVector(ctx, &sub, &at->parameters);
    } else if (field == kAt_ctad_names_template_template_parameter) {
      WireReadBool(&sub, &at->ctad_names_template_template_parameter);
    } else {
      WireSkip(&sub, wt);
    }
  }
  return at;
}

// ---------------------------------------------------------------------------
// Symbol.
// ---------------------------------------------------------------------------
static void WriteBoolField(WireBuffer* buf, int field, bool value) {
  WireWriteBool(buf, field, value);
}

static bool SymbolVisibleInModuleArtifact(SerializeContext* ctx,
                                          Symbol* symbol) {
  if (symbol == NULL || symbol->import_source_module.length != 0) {
    return false;
  }
  if (ctx->writing_internal_partition) {
    return symbol->owning_module_name.length != 0 &&
           symbol->cxx_linkage != kCXXLinkageInternal;
  }
  return symbol->flags.is_exported;
}

static Symbol* NextVisibleOverload(SerializeContext* ctx, Symbol* symbol) {
  for (Symbol* current = symbol; current != NULL;
       current = current->overload_next) {
    if (SymbolVisibleInModuleArtifact(ctx, current)) {
      return current;
    }
  }
  return NULL;
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
  WriteBoolField(buf, kSym_is_template_template_parameter,
                 s->flags.is_template_template_parameter);
  WriteBoolField(buf, kSym_is_parameter_pack, s->flags.is_parameter_pack);
  WriteBoolField(buf, kSym_is_constexpr, s->flags.is_constexpr);
  WriteBoolField(buf, kSym_is_constinit, s->flags.is_constinit);
  WriteBoolField(buf, kSym_is_weak, s->flags.is_weak);
  WriteBoolField(buf, kSym_is_c_linkage, s->flags.is_c_linkage);
  WriteBoolField(buf, kSym_is_exported, s->flags.is_exported);
  WriteBoolField(buf, kSym_is_concept, s->flags.is_concept);
  WriteBoolField(buf, kSym_is_module_private, s->flags.is_module_private);
  WriteBoolField(buf, kSym_is_explicit_specialization,
                 s->flags.is_explicit_specialization);
  WriteBoolField(buf, kSym_is_name_independent,
                 s->flags.is_name_independent);
  WriteBoolField(buf, kSym_name_independent_lookup_ambiguous,
                 s->flags.name_independent_lookup_ambiguous);
  if (s->template_template_parameters != NULL) {
    SerialWriteTemplateParameterVector(
        ctx, buf, kSym_template_template_parameters,
        s->template_template_parameters);
  }
  WireWriteInt32(buf, kSym_template_template_parameter_kind,
                 (int32_t)s->template_template_parameter_kind);

  WireWriteInt32(buf, kSym_alignment, s->alignment);
  WireWriteInt32(buf, kSym_template_parameter_index,
                 s->template_parameter_index);
  WireWriteInt32(buf, kSym_dependent_value_template_parameter_index,
                 s->dependent_value_template_parameter_index);
  WireWriteUint64(buf, kSym_location, (uint64_t)s->location);
  bool constexpr_object =
      s->flags.value_set && s->type != NULL &&
      (TypeIsFixedArray(s->type) || TypeIsStructOrUnion(s->type));
  bool constexpr_reflection =
      s->flags.value_set && s->type != NULL &&
      TypeIsReflection(s->type);
  if (constexpr_object) {
    ASTNode* semantic_initializer =
        ConstexprObjectInitializerForSymbol(s, s->location);
    if (semantic_initializer != NULL) {
      s->constexpr_initializer = semantic_initializer;
    }
  }
  if (constexpr_reflection && s->constexpr_initializer == NULL &&
      s->value.other != NULL) {
    s->constexpr_initializer = NewReflectionConstantASTNode(
        (ReflectionValue*)s->value.other, s->location);
  }
  if (s->type == NULL || !TypeIsFunction(s->type)) {
    WireWriteInt64(buf, kSym_value_ivalue,
                   constexpr_object || constexpr_reflection
                       ? 0
                       : s->value.ivalue);
  }
  WireWriteInt32(buf, kSym_stack_offset, s->stack_offset);
  SWriteRef(ctx, buf, kSym_alias_target, kSerialKindSymbol, s->alias_target);
  Symbol* overload_next =
      ctx->writing_module_interface &&
              SymbolVisibleInModuleArtifact(ctx, s)
          ? NextVisibleOverload(ctx, s->overload_next)
          : s->overload_next;
  SWriteRef(ctx, buf, kSym_overload_next, kSerialKindSymbol, overload_next);
  if (s->type != NULL && TypeIsFunction(s->type) &&
      s->value.func_defn != NULL &&
      s->value.func_defn != s) {
    SWriteRef(ctx, buf, kSym_func_defn, kSerialKindSymbol,
              s->value.func_defn);
  }
  SWriteRef(ctx, buf, kSym_default_argument, kSerialKindAST,
            s->default_argument);
  SWriteRef(ctx, buf, kSym_constexpr_initializer, kSerialKindAST,
            s->constexpr_initializer);
  SerialWriteAttributeVector(ctx, buf, kSym_attributes, &s->attributes);
  if (s->variable_template != NULL) {
    WriteVariableTemplate(ctx, buf, kSym_variable_template,
                          s->variable_template);
  }
  if (s->alias_template != NULL) {
    WriteAliasTemplate(ctx, buf, kSym_alias_template, s->alias_template);
  }
  SerialWriteConcept(ctx, buf, kSym_concept_definition, s->concept_definition);
  SerialWriteConstraint(ctx, buf, kSym_associated_constraint,
                        s->associated_constraint);
  WriteTemplateConstructorInitializers(
      ctx, buf, kSym_template_constructor_initializers, s);
  WireWriteInt32(buf, kSym_cxx_linkage, (int32_t)s->cxx_linkage);
  SWriteStringVal(ctx, buf, kSym_owning_module_name, &s->owning_module_name);
  SWriteStringVal(ctx, buf, kSym_owning_module_partition,
                  &s->owning_module_partition);
  SWriteStringVal(ctx, buf, kSym_import_source_module,
                  &s->import_source_module);
  return !WireBufferHasError(buf);
}

static void* AllocSymbol(DeserializeContext* ctx, const void* blob,
                         size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  Symbol* symbol = NewSymbol("", NULL, STO(implicit));
  symbol->is_imported_module_symbol = true;
  return symbol;
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
      case kSym_is_template_template_parameter:
        WireReadBool(buf, &b);
        s->flags.is_template_template_parameter = b;
        break;
      case kSym_template_template_parameters:
        s->template_template_parameters = NewVector();
        SerialReadTemplateParameterVector(
            ctx, buf, s->template_template_parameters);
        break;
      case kSym_template_template_parameter_kind: {
        int32_t v;
        WireReadInt32(buf, &v);
        s->template_template_parameter_kind =
            (TemplateTemplateParameterKind)v;
        break;
      }
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
      case kSym_constexpr_initializer:
        s->constexpr_initializer =
            (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kSym_attributes:
        SerialReadAttributeVector(ctx, buf, &s->attributes);
        break;
      case kSym_variable_template:
        s->variable_template = ReadVariableTemplate(ctx, buf);
        break;
      case kSym_concept_definition:
        s->concept_definition = SerialReadConcept(ctx, buf);
        break;
      case kSym_associated_constraint:
        s->associated_constraint = SerialReadConstraint(ctx, buf);
        break;
      case kSym_alias_template:
        s->alias_template = ReadAliasTemplate(ctx, buf);
        break;
      case kSym_cxx_linkage: {
        int32_t v = 0;
        WireReadInt32(buf, &v);
        s->cxx_linkage = (CXXLinkageKind)v;
        break;
      }
      case kSym_owning_module_name:
        SReadStringVal(ctx, buf, &s->owning_module_name);
        break;
      case kSym_owning_module_partition:
        SReadStringVal(ctx, buf, &s->owning_module_partition);
        break;
      case kSym_import_source_module:
        SReadStringVal(ctx, buf, &s->import_source_module);
        break;
      case kSym_is_module_private:
        WireReadBool(buf, &b);
        s->flags.is_module_private = b;
        break;
      case kSym_is_explicit_specialization:
        WireReadBool(buf, &b);
        s->flags.is_explicit_specialization = b;
        break;
      case kSym_is_name_independent:
        WireReadBool(buf, &b);
        s->flags.is_name_independent = b;
        break;
      case kSym_name_independent_lookup_ambiguous:
        WireReadBool(buf, &b);
        s->flags.name_independent_lookup_ambiguous = b;
        break;
      case kSym_func_defn:
        s->value.func_defn =
            (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kSym_template_constructor_initializers:
        ReadTemplateConstructorInitializers(ctx, buf, s);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  if (s->type != NULL && TypeIsFunction(s->type)) {
    if (s->type->info.function.symbol == NULL) {
      s->type->info.function.symbol = s;
    }
    if (s->type->info.function.body != NULL && s->value.func_defn == NULL) {
      s->value.func_defn = s;
    }
  }
  if (s->constexpr_initializer != NULL && s->type != NULL &&
      (TypeIsFixedArray(s->type) || TypeIsStructOrUnion(s->type) ||
       TypeIsReflection(s->type))) {
    s->flags.value_set = false;
    s->value.other = NULL;
  }
  TypeRecordIncRef(s->type);
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

typedef struct {
  SerializeContext* ctx;
  Vector* symbols;
} VisibleSymbolCollector;

static void CollectVisibleSymbol(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  VisibleSymbolCollector* collector = (VisibleSymbolCollector*)data;
  Symbol* symbol = ((SymbolNode*)node)->symbol;
  Symbol* first_visible = NextVisibleOverload(collector->ctx, symbol);
  if (first_visible != NULL) {
    VectorAppend(collector->symbols, first_visible);
  }
}

static bool NamespaceHasVisibleSurface(SerializeContext* ctx, Namespace* ns) {
  if (ns == NULL || ns->is_anonymous) {
    return false;
  }
  Vector exported;
  VectorInit(&exported);
  VisibleSymbolCollector collector = {.ctx = ctx, .symbols = &exported};
  BinaryTreeTraverse(&ns->symbol_table, CollectVisibleSymbol, &collector);
  BinaryTreeTraverse(&ns->tag_table, CollectVisibleSymbol, &collector);
  bool result = exported.length > 0;
  VectorDestruct(&exported);
  if (result) {
    return true;
  }
  for (size_t i = 0; i < ns->namespace_aliases.length; i++) {
    NamespaceAlias* alias =
        (NamespaceAlias*)VectorGet(&ns->namespace_aliases, i);
    if (alias != NULL && !alias->is_imported_module_alias &&
        (ctx->writing_internal_partition || alias->is_exported)) {
      return true;
    }
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    if (NamespaceHasVisibleSurface(
            ctx, (Namespace*)VectorGet(&ns->children, i))) {
      return true;
    }
  }
  return false;
}

static bool WriteNamespace(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  Namespace* ns = (Namespace*)obj;
  SWriteStringVal(ctx, buf, kNs_name, &ns->name);
  SWriteStringVal(ctx, buf, kNs_qualified_name, &ns->qualified_name);
  WireWriteBool(buf, kNs_is_anonymous, ns->is_anonymous);
  WireWriteBool(buf, kNs_is_inline, ns->is_inline);
  Vector visible_children;
  VectorInit(&visible_children);
  if (ctx->writing_module_interface) {
    for (size_t i = 0; i < ns->children.length; i++) {
      Namespace* child = (Namespace*)VectorGet(&ns->children, i);
      if (NamespaceHasVisibleSurface(ctx, child)) {
        VectorAppend(&visible_children, child);
      }
    }
  }
  SWriteRefVector(ctx, buf, kNs_children, kSerialKindNamespace,
                  ctx->writing_module_interface ? &visible_children
                                                : &ns->children);
  VectorDestruct(&visible_children);
  SWriteRef(ctx, buf, kNs_parent, kSerialKindNamespace, ns->parent);
  if (!ctx->writing_module_interface) {
    SWriteRef(ctx, buf, kNs_anonymous_child, kSerialKindNamespace,
              ns->anonymous_child);
  }

  Vector alias_names;
  Vector alias_targets;
  VectorInit(&alias_names);
  VectorInit(&alias_targets);
  for (size_t i = 0; i < ns->namespace_aliases.length; i++) {
    NamespaceAlias* alias =
        (NamespaceAlias*)VectorGet(&ns->namespace_aliases, i);
    if (alias != NULL &&
        (!ctx->writing_module_interface ||
         (!alias->is_imported_module_alias &&
          (ctx->writing_internal_partition || alias->is_exported)))) {
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
  if (ctx->writing_module_interface) {
    VisibleSymbolCollector collector = {.ctx = ctx, .symbols = &symbols};
    BinaryTreeTraverse(&ns->symbol_table, CollectVisibleSymbol, &collector);
  } else {
    BinaryTreeTraverse(&ns->symbol_table, CollectSymbol, &symbols);
  }
  SWriteRefVector(ctx, buf, kNs_symbols, kSerialKindSymbol, &symbols);
  VectorDestruct(&symbols);

  Vector tags;
  VectorInit(&tags);
  if (ctx->writing_module_interface) {
    VisibleSymbolCollector collector = {.ctx = ctx, .symbols = &tags};
    BinaryTreeTraverse(&ns->tag_table, CollectVisibleSymbol, &collector);
  } else {
    BinaryTreeTraverse(&ns->tag_table, CollectSymbol, &tags);
  }
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
