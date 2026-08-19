//
//  type_serialize.c
//  c_compiler
//
//  Serialization of the type graph: TypeRecord, Struct, StructMember, Enum, and
//  their inline sub-objects (ArrayInfo, FunctionInfo, TemplateArgument,
//  TemplateParameter, CXXBaseSpecifier).  Cross-references between pooled
//  objects are written as integer handles (see serialize.h).
//
//  Field numbers are documented here and mirrored by `// @wire N` comments in
//  type.h.  Codegen-only / transient fields (refs counts, codegen_info, DIEs,
//  symbol-table maps) are intentionally omitted; they are recomputed on load.
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "constraint_serialize.h"
#include "reflection.h"
#include "serialize_common.h"
#include "type.h"
#include "type_internal.h"

//
// TypeRecord field numbers.
//
enum {
  kType_id = 1,
  kType_type = 2,
  kType_qualifiers = 3,
  kType_declarator = 4,
  kType_size = 5,
  kType_template_parameter_index = 6,
  kType_dependent_member_name = 7,
  kType_template_origin = 8,
  kType_template_arguments = 9,
  kType_next = 10,
  kType_array = 11,
  kType_function = 12,
  kType_struct_info = 13,
  kType_enum_info = 14,
  kType_dependent_decltype_expr = 15,
  kType_template_parameter_name = 16,
  kType_dependent_member_template_arguments = 17,
  kType_is_pack_index = 18,
  kType_pack_index_expr = 19,
  kType_dependent_splice_expr = 20,
  kType_bit_width = 21,
};

static const WireFieldDesc kTypeFields[] = {
    {kType_id, "id"},
    {kType_type, "type"},
    {kType_qualifiers, "qualifiers"},
    {kType_declarator, "declarator"},
    {kType_size, "size"},
    {kType_template_parameter_index, "template_parameter_index"},
    {kType_dependent_member_name, "dependent_member_name"},
    {kType_template_origin, "template_origin"},
    {kType_template_arguments, "template_arguments"},
    {kType_next, "next"},
    {kType_array, "array"},
    {kType_function, "function"},
    {kType_struct_info, "struct_info"},
    {kType_enum_info, "enum_info"},
    {kType_dependent_decltype_expr, "dependent_decltype_expr"},
    {kType_template_parameter_name, "template_parameter_name"},
    {kType_dependent_member_template_arguments,
     "dependent_member_template_arguments"},
    {kType_is_pack_index, "is_pack_index"},
    {kType_pack_index_expr, "pack_index_expr"},
    {kType_dependent_splice_expr, "dependent_splice_expr"},
    {kType_bit_width, "bit_width"},
};

//
// TemplateArgument (inline sub-message) field numbers.
//
enum {
  kTArg_kind = 1,
  kTArg_is_pack_expansion = 2,
  kTArg_type = 3,
  kTArg_int_value = 4,
  kTArg_template_parameter_index = 5,
  kTArg_pack_arguments = 6,
  kTArg_dependent_expr = 7,
  kTArg_location = 8,
  kTArg_value_kind = 9,
  kTArg_value_symbol = 10,
  kTArg_value_offset = 11,
  kTArg_value_adjustment = 12,
  kTArg_member_function = 13,
  kTArg_template_symbol = 14,
  kTArg_references_parameter_pack = 15,
  kTArg_reflection_kind = 16,
  kTArg_reflection_type = 17,
  kTArg_reflection_symbol = 18,
  kTArg_reflection_member = 19,
  kTArg_reflection_namespace = 20,
  kTArg_reflection_parent = 21,
  kTArg_reflection_base_index = 22,
  kTArg_reflection_location = 23,
  kTArg_reflection_parameter_index = 24,
  kTArg_reflection_scalar_ivalue = 25,
  kTArg_reflection_scalar_fvalue = 26,
  kTArg_reflection_scalar_is_float = 27,
  kTArg_reflection_namespace_alias_target = 28,
  kTArg_reflection_constexpr_initializer = 29,
  kTArg_reflection_promoted_symbol = 30,
  kTArg_reflection_substituted_template = 31,
  kTArg_reflection_extract_type = 32,
  kTArg_reflection_dms_member_type = 33,
  kTArg_reflection_dms_name = 34,
  kTArg_reflection_dms_alignment = 35,
  kTArg_reflection_dms_bit_width = 36,
  kTArg_reflection_dms_no_unique_address = 37,
  kTArg_reflection_dms_has_name = 38,
  kTArg_reflection_dms_has_alignment = 39,
  kTArg_reflection_dms_has_bit_width = 40,
  kTArg_reflection_sequence = 41,
  kTArg_reflection_substituted_arguments = 42,
  kTArg_reflection_dms_annotations = 43,
  kTArg_object_initializer = 44,
  kTArg_reflection_token_sequence = 45,
  kTArg_reflection_ens_name = 46,
  kTArg_reflection_ens_value = 47,
  kTArg_reflection_ens_has_value = 48,
  kTArg_reflection_ens_has_name = 49,
  kTArg_reflection_ens_attributes = 50,
  kTArg_reflection_ens_annotations = 51,
};

enum {
  kTokenSeqWire_kind = 1,
  kTokenSeqWire_spelling = 2,
  kTokenSeqWire_location = 3,
  kTokenSeqWire_pseudo_value = 4,
  kTokenSeqWire_piece_kind = 5,
};

//
// TemplateParameter (inline sub-message) field numbers.
//
enum {
  kTParam_name = 1,
  kTParam_kind = 2,
  kTParam_is_parameter_pack = 3,
  kTParam_type = 4,
  kTParam_default_type = 5,
  kTParam_has_default_int = 6,
  kTParam_default_int_value = 7,
  kTParam_default_template_parameter_index = 8,
  kTParam_index = 9,
  kTParam_associated_constraint = 10,
  kTParam_default_argument = 11,
  kTParam_template_parameters = 12,
  kTParam_template_template_kind = 13,
};

//
// ArrayInfo (inline sub-message) field numbers.
//
enum {
  kArr_is_flexible = 1,
  kArr_is_static = 2,
  kArr_is_vla = 3,
  kArr_is_placeholder_vla = 4,
  kArr_template_parameter_index = 5,
  kArr_fixed = 6,
  kArr_vla_size = 7,
  kArr_is_dependent_bound = 8,
};

//
// FunctionInfo (inline sub-message) field numbers.
//
enum {
  kFn_symbol = 1,
  kFn_prototype = 2,
  kFn_varargs = 3,
  kFn_body = 4,
  kFn_unknown_args = 5,
  kFn_definition = 6,
  kFn_old_style = 7,
  kFn_is_inline = 8,
  kFn_is_constexpr = 9,
  kFn_is_consteval = 10,
  kFn_is_constructor = 11,
  kFn_is_destructor = 12,
  kFn_is_const_member = 13,
  kFn_ref_qualifier = 14,
  kFn_is_explicit = 15,
  kFn_is_explicit_conversion = 16,
  kFn_is_virtual = 17,
  kFn_is_override = 18,
  kFn_is_final = 19,
  kFn_is_pure_virtual = 20,
  kFn_is_defaulted = 21,
  kFn_is_deleted = 22,
  kFn_cxx_special_member_kind = 23,
  kFn_is_user_declared = 24,
  kFn_is_user_provided = 25,
  kFn_is_explicitly_defaulted = 26,
  kFn_is_explicitly_deleted = 27,
  kFn_is_implicitly_declared = 28,
  kFn_is_implicitly_deleted = 29,
  kFn_is_trivial_special_member = 30,
  kFn_is_constexpr_eligible = 31,
  kFn_is_noexcept_eligible = 32,
  kFn_is_noexcept = 33,
  kFn_is_auto_return_deduced = 34,
  kFn_is_deduction_guide = 35,
  kFn_is_coroutine = 36,
  kFn_coroutine_promise_type = 37,
  kFn_coroutine_frame_type = 38,
  kFn_coroutine_suspend_count = 39,
  kFn_virtual_index = 40,
  kFn_cxx_member_owner = 41,
  kFn_template_origin = 42,
  kFn_template_parameter_count = 43,
  kFn_template_parameter_base = 44,
  kFn_template_parameters = 45,
  kFn_template_instantiations = 46,
  kFn_associated_constraint = 47,
  kFn_explicit_condition = 48,
  kFn_is_volatile_member = 49,
  kFn_has_explicit_object_parameter = 50,
  kFn_is_decltype_auto_return_deduced = 51,
  kFn_deleted_reason = 52,
  kFn_contract_assertions = 53,
};

//
// CXXBaseSpecifier (inline sub-message) field numbers.
//
enum {
  kBase_type = 1,
  kBase_access = 2,
  kBase_byte_offset = 3,
  kBase_is_virtual = 4,
  kBase_is_pack_expansion = 5,
};

//
// CXXMemberUsingDeclaration (inline sub-message) field numbers.
//
enum {
  kMemberUsing_base_type = 1,
  kMemberUsing_member_name = 2,
  kMemberUsing_access = 3,
  kMemberUsing_location = 4,
  kMemberUsing_is_pack_expansion = 5,
  kMemberUsing_qualifier_names_constructor = 6,
};

//
// CXXFriendTypeDeclaration (inline sub-message) field numbers.
//
enum {
  kFriendType_type = 1,
  kFriendType_location = 2,
  kFriendType_is_pack_expansion = 3,
};

//
// Enum field numbers.
//
enum {
  kEnum_tag_name = 1,
  kEnum_tag_symbol = 2,
  kEnum_constants = 3,
  kEnum_next_value = 4,
  kEnum_is_scoped = 5,
  kEnum_has_fixed_underlying = 6,
  kEnum_fixed_underlying_type = 7,
  kEnum_fixed_underlying_size = 8,
  kEnum_fixed_underlying_bit_width = 9,
};

static const WireFieldDesc kEnumFields[] = {
    {kEnum_tag_name, "tag_name"},
    {kEnum_tag_symbol, "tag_symbol"},
    {kEnum_constants, "constants"},
    {kEnum_next_value, "next_value"},
    {kEnum_is_scoped, "is_scoped"},
    {kEnum_has_fixed_underlying, "has_fixed_underlying"},
    {kEnum_fixed_underlying_type, "fixed_underlying_type"},
    {kEnum_fixed_underlying_size, "fixed_underlying_size"},
    {kEnum_fixed_underlying_bit_width, "fixed_underlying_bit_width"},
};

//
// StructMember field numbers.
//
enum {
  kMem_symbol = 1,
  kMem_default_initializer = 2,
  kMem_byte_offset = 3,
  kMem_bit_offset = 4,
  kMem_bit_size = 5,
  kMem_index = 6,
  kMem_cxx_vcall_offset = 7,
  kMem_is_anon = 8,
  kMem_is_static = 9,
  kMem_is_mutable = 10,
  kMem_is_member_function = 11,
  kMem_is_using_declaration = 12,
  kMem_access = 13,
  kMem_overload_next = 14,
};

static const WireFieldDesc kMemberFields[] = {
    {kMem_symbol, "symbol"},
    {kMem_default_initializer, "default_initializer"},
    {kMem_byte_offset, "byte_offset"},
    {kMem_bit_offset, "bit_offset"},
    {kMem_bit_size, "bit_size"},
    {kMem_index, "index"},
    {kMem_cxx_vcall_offset, "cxx_vcall_offset"},
    {kMem_is_anon, "is_anon"},
    {kMem_is_static, "is_static"},
    {kMem_is_mutable, "is_mutable"},
    {kMem_is_member_function, "is_member_function"},
    {kMem_is_using_declaration, "is_using_declaration"},
    {kMem_access, "access"},
    {kMem_overload_next, "overload_next"},
};

//
// Struct field numbers.
//
enum {
  kStruct_tag_name = 1,
  kStruct_tag_symbol = 2,
  kStruct_bases = 3,
  kStruct_members = 4,
  kStruct_next_offset = 5,
  kStruct_size = 6,
  kStruct_non_virtual_size = 7,
  kStruct_alignment = 8,
  kStruct_is_union = 9,
  kStruct_is_class = 10,
  kStruct_is_final = 11,
  kStruct_is_template = 12,
  kStruct_is_aggregate = 13,
  kStruct_cxx_special_members_complete = 14,
  kStruct_template_parameters = 15,
  kStruct_template_parameter_count = 16,
  kStruct_packed = 17,
  kStruct_is_abstract = 18,
  kStruct_explicit_alignment = 19,
  kStruct_pack = 20,
  kStruct_next_bit_pos = 21,
  kStruct_current_offset = 22,
  kStruct_vptr_member = 23,
  kStruct_vtable_symbol = 24,
  kStruct_vbptr_member = 25,
  kStruct_vbtable_symbol = 26,
  kStruct_virtual_members = 27,
  kStruct_friend_classes = 28,
  kStruct_friend_functions = 29,
  kStruct_associated_constraint = 30,
  kStruct_lexical_parent = 31,
  kStruct_partial_specializations = 32,
  kStruct_deduction_guides = 33,
  kStruct_member_using_declarations = 34,
  kStruct_friend_type_declarations = 35,
  kStruct_meta_aggregate_complete = 36,
};

static const WireFieldDesc kStructFields[] = {
    {kStruct_tag_name, "tag_name"},
    {kStruct_tag_symbol, "tag_symbol"},
    {kStruct_bases, "bases"},
    {kStruct_members, "members"},
    {kStruct_next_offset, "next_offset"},
    {kStruct_size, "size"},
    {kStruct_non_virtual_size, "non_virtual_size"},
    {kStruct_alignment, "alignment"},
    {kStruct_is_union, "is_union"},
    {kStruct_is_class, "is_class"},
    {kStruct_is_final, "is_final"},
    {kStruct_is_template, "is_template"},
    {kStruct_is_aggregate, "is_aggregate"},
    {kStruct_cxx_special_members_complete, "cxx_special_members_complete"},
    {kStruct_template_parameters, "template_parameters"},
    {kStruct_template_parameter_count, "template_parameter_count"},
    {kStruct_packed, "packed"},
    {kStruct_is_abstract, "is_abstract"},
    {kStruct_explicit_alignment, "explicit_alignment"},
    {kStruct_pack, "pack"},
    {kStruct_next_bit_pos, "next_bit_pos"},
    {kStruct_current_offset, "current_offset"},
    {kStruct_vptr_member, "vptr_member"},
    {kStruct_vtable_symbol, "vtable_symbol"},
    {kStruct_vbptr_member, "vbptr_member"},
    {kStruct_vbtable_symbol, "vbtable_symbol"},
    {kStruct_virtual_members, "virtual_members"},
    {kStruct_friend_classes, "friend_classes"},
    {kStruct_friend_functions, "friend_functions"},
    {kStruct_associated_constraint, "associated_constraint"},
    {kStruct_lexical_parent, "lexical_parent"},
    {kStruct_partial_specializations, "partial_specializations"},
    {kStruct_deduction_guides, "deduction_guides"},
    {kStruct_member_using_declarations, "member_using_declarations"},
    {kStruct_friend_type_declarations, "friend_type_declarations"},
    {kStruct_meta_aggregate_complete, "meta_aggregate_complete"},
};

// ---------------------------------------------------------------------------
// TemplateParameter (inline).
// ---------------------------------------------------------------------------
static void WriteTemplateArgument(SerializeContext* ctx, WireBuffer* out,
                                  TemplateArgument* a);
static TemplateArgument* ReadTemplateArgument(DeserializeContext* ctx,
                                              WireBuffer* in);

static void WriteTemplateArgumentField(SerializeContext* ctx, WireBuffer* out,
                                       int field, TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  WireBuffer sub;
  WireBufferInitOwned(&sub, 32);
  WriteTemplateArgument(ctx, &sub, arg);
  WireWriteBytes(out, field, WireBufferData(&sub), WireBufferSize(&sub));
  WireBufferDestruct(&sub);
}

static TemplateArgument* ReadTemplateArgumentField(DeserializeContext* ctx,
                                                   WireBuffer* in) {
  const void* data;
  size_t length;
  if (!WireReadBytes(in, &data, &length)) {
    return NULL;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, length);
  return ReadTemplateArgument(ctx, &sub);
}

static void WriteTemplateParameter(SerializeContext* ctx, WireBuffer* out,
                                   TemplateParameter* p) {
  SWriteStringVal(ctx, out, kTParam_name, &p->name);
  WireWriteInt32(out, kTParam_kind, (int32_t)p->kind);
  WireWriteBool(out, kTParam_is_parameter_pack, p->is_parameter_pack);
  SWriteRef(ctx, out, kTParam_type, kSerialKindType, p->type);
  SWriteRef(ctx, out, kTParam_default_type, kSerialKindType, p->default_type);
  WireWriteBool(out, kTParam_has_default_int, p->has_default_int);
  WireWriteInt64(out, kTParam_default_int_value, p->default_int_value);
  WireWriteInt32(out, kTParam_default_template_parameter_index,
                 p->default_template_parameter_index);
  WireWriteInt32(out, kTParam_index, p->index);
  SerialWriteConstraint(ctx, out, kTParam_associated_constraint,
                        p->associated_constraint);
  WriteTemplateArgumentField(ctx, out, kTParam_default_argument,
                             p->default_argument);
  if (p->template_parameters != NULL) {
    SerialWriteTemplateParameterVector(
        ctx, out, kTParam_template_parameters, p->template_parameters);
  }
  WireWriteInt32(out, kTParam_template_template_kind,
                 (int32_t)p->template_template_kind);
}

static TemplateParameter* ReadTemplateParameter(DeserializeContext* ctx,
                                                WireBuffer* in) {
  TemplateParameter* p = (TemplateParameter*)calloc(1, sizeof(*p));
  StringInit(&p->name, NULL);
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kTParam_name:
        SReadStringVal(ctx, in, &p->name);
        break;
      case kTParam_kind: {
        int32_t v;
        WireReadInt32(in, &v);
        p->kind = (TemplateParameterKind)v;
        break;
      }
      case kTParam_is_parameter_pack:
        WireReadBool(in, &p->is_parameter_pack);
        break;
      case kTParam_type:
        p->type = (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kTParam_default_type:
        p->default_type = (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kTParam_has_default_int:
        WireReadBool(in, &p->has_default_int);
        break;
      case kTParam_default_int_value:
        WireReadInt64(in, &p->default_int_value);
        break;
      case kTParam_default_template_parameter_index:
        WireReadInt32(in, &p->default_template_parameter_index);
        break;
      case kTParam_index:
        WireReadInt32(in, &p->index);
        break;
      case kTParam_associated_constraint:
        p->associated_constraint = SerialReadConstraint(ctx, in);
        break;
      case kTParam_default_argument:
        p->default_argument = ReadTemplateArgumentField(ctx, in);
        break;
      case kTParam_template_parameters:
        p->template_parameters = NewVector();
        SerialReadTemplateParameterVector(ctx, in, p->template_parameters);
        break;
      case kTParam_template_template_kind: {
        int32_t v;
        WireReadInt32(in, &v);
        p->template_template_kind = (TemplateTemplateParameterKind)v;
        break;
      }
      default:
        WireSkip(in, wt);
        break;
    }
  }
  if (p->default_argument == NULL && p->has_default_int) {
    p->default_argument = NewIntegralTemplateArgument(p->default_int_value);
    p->default_argument->template_parameter_index =
        p->default_template_parameter_index;
    if (p->default_template_parameter_index >= 0) {
      p->default_argument->value_kind = kTemplateValueNone;
    }
  }
  return p;
}

// A vector<TemplateParameter*> written as a length-delimited blob:
// [count][ each: length-delimited sub-message ].  `v` is an embedded Vector.
// Exposed (non-static) so symbol_serialize.c can (de)serialize variable
// template parameter lists.
void SerialWriteTemplateParameterVector(SerializeContext* ctx, WireBuffer* buf,
                                        int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    WriteTemplateParameter(ctx, &elem, (TemplateParameter*)VectorGet(v, i));
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

void SerialReadTemplateParameterVector(DeserializeContext* ctx, WireBuffer* in,
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
    VectorAppend(out, ReadTemplateParameter(ctx, &er));
  }
}

// ---------------------------------------------------------------------------
// TemplateArgument (inline, recursive through pack_arguments).
// ---------------------------------------------------------------------------
static void WriteTemplateArgument(SerializeContext* ctx, WireBuffer* out,
                                  TemplateArgument* a);
static void WriteTemplateArgumentVector(SerializeContext* ctx, WireBuffer* buf,
                                        int field, Vector* v);
static Vector* ReadTemplateArgumentVector(DeserializeContext* ctx,
                                          WireBuffer* in);

static void WriteTemplateArgument(SerializeContext* ctx, WireBuffer* out,
                                  TemplateArgument* a) {
  WireWriteInt32(out, kTArg_kind, (int32_t)a->kind);
  WireWriteBool(out, kTArg_is_pack_expansion, a->is_pack_expansion);
  WireWriteBool(out, kTArg_references_parameter_pack,
                a->references_parameter_pack);
  SWriteRef(ctx, out, kTArg_type, kSerialKindType, a->type);
  WireWriteInt64(out, kTArg_int_value, a->int_value);
  WireWriteInt32(out, kTArg_template_parameter_index,
                 a->template_parameter_index);
  if (a->pack_arguments != NULL) {
    WriteTemplateArgumentVector(ctx, out, kTArg_pack_arguments,
                                a->pack_arguments);
  }
  SWriteRef(ctx, out, kTArg_dependent_expr, kSerialKindAST, a->dependent_expr);
  WireWriteUint64(out, kTArg_location, (uint64_t)a->location);
  WireWriteInt32(out, kTArg_value_kind, (int32_t)a->value_kind);
  SWriteRef(ctx, out, kTArg_value_symbol, kSerialKindSymbol, a->value_symbol);
  WireWriteInt64(out, kTArg_value_offset, a->value_offset);
  WireWriteInt64(out, kTArg_value_adjustment, a->value_adjustment);
  SWriteRef(ctx, out, kTArg_member_function, kSerialKindSymbol,
            a->member_function);
  SWriteRef(ctx, out, kTArg_template_symbol, kSerialKindSymbol,
            a->template_symbol);
  SWriteRef(ctx, out, kTArg_object_initializer, kSerialKindAST,
            a->object_initializer);
  if (a->reflection_value != NULL) {
    ReflectionValue* reflection = a->reflection_value;
    WireWriteInt32(out, kTArg_reflection_kind,
                   (int32_t)reflection->kind);
    SWriteRef(ctx, out, kTArg_reflection_type, kSerialKindType,
              reflection->reflected_type);
    SWriteRef(ctx, out, kTArg_reflection_symbol, kSerialKindSymbol,
              reflection->symbol);
    SWriteRef(ctx, out, kTArg_reflection_member, kSerialKindStructMember,
              reflection->member);
    SWriteRef(ctx, out, kTArg_reflection_namespace, kSerialKindNamespace,
              reflection->namespace_);
    SWriteRef(ctx, out, kTArg_reflection_parent, kSerialKindStruct,
              reflection->parent_class);
    WireWriteUint64(out, kTArg_reflection_base_index,
                    (uint64_t)reflection->base_index);
    WireWriteUint64(out, kTArg_reflection_location,
                    (uint64_t)reflection->location);
    WireWriteUint64(out, kTArg_reflection_parameter_index,
                    (uint64_t)reflection->parameter_index);
    WireWriteInt64(out, kTArg_reflection_scalar_ivalue,
                   reflection->scalar_ivalue);
    WireWriteDouble(out, kTArg_reflection_scalar_fvalue,
                    reflection->scalar_fvalue);
    WireWriteBool(out, kTArg_reflection_scalar_is_float,
                  reflection->scalar_is_float);
    SWriteRef(ctx, out, kTArg_reflection_namespace_alias_target,
              kSerialKindNamespace, reflection->namespace_alias_target);
    SWriteRef(ctx, out, kTArg_reflection_constexpr_initializer, kSerialKindAST,
              reflection->constexpr_initializer);
    SWriteRef(ctx, out, kTArg_reflection_promoted_symbol, kSerialKindSymbol,
              reflection->promoted_symbol);
    SWriteRef(ctx, out, kTArg_reflection_substituted_template, kSerialKindSymbol,
              reflection->substituted_template);
    SWriteRef(ctx, out, kTArg_reflection_extract_type, kSerialKindType,
              reflection->extract_type);
    if (reflection->data_member_spec != NULL) {
      ReflectionDataMemberSpec* spec = reflection->data_member_spec;
      SWriteRef(ctx, out, kTArg_reflection_dms_member_type, kSerialKindType,
                spec->member_type);
      if (spec->name.value != NULL) {
        WireWriteString(out, kTArg_reflection_dms_name, spec->name.value,
                        spec->name.length);
      }
      WireWriteUint64(out, kTArg_reflection_dms_alignment,
                      (uint64_t)spec->alignment);
      WireWriteUint64(out, kTArg_reflection_dms_bit_width,
                      (uint64_t)spec->bit_width);
      WireWriteBool(out, kTArg_reflection_dms_no_unique_address,
                    spec->no_unique_address);
      WireWriteBool(out, kTArg_reflection_dms_has_name, spec->has_name);
      WireWriteBool(out, kTArg_reflection_dms_has_alignment, spec->has_alignment);
      WireWriteBool(out, kTArg_reflection_dms_has_bit_width, spec->has_bit_width);
    }
    if (reflection->enumerator_spec != NULL) {
      ReflectionEnumeratorSpec* spec = reflection->enumerator_spec;
      if (spec->name.value != NULL) {
        SWriteStringVal(ctx, out, kTArg_reflection_ens_name, &spec->name);
      }
      WireWriteInt64(out, kTArg_reflection_ens_value, spec->value);
      WireWriteBool(out, kTArg_reflection_ens_has_value, spec->has_value);
      WireWriteBool(out, kTArg_reflection_ens_has_name, spec->has_name);
    }
    SerialWriteReflectionExtendedPayload(
        ctx, out, kTArg_reflection_sequence,
        kTArg_reflection_substituted_arguments, kTArg_reflection_dms_annotations,
        kTArg_reflection_token_sequence, reflection);
  }
}

static TemplateArgument* ReadTemplateArgument(DeserializeContext* ctx,
                                              WireBuffer* in) {
  TemplateArgument* a = (TemplateArgument*)calloc(1, sizeof(*a));
  a->template_parameter_index = -1;
  a->location = SOURCE_LOCATION_MISSING;
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kTArg_kind: {
        int32_t v;
        WireReadInt32(in, &v);
        a->kind = (TemplateParameterKind)v;
        break;
      }
      case kTArg_is_pack_expansion:
        WireReadBool(in, &a->is_pack_expansion);
        break;
      case kTArg_references_parameter_pack:
        WireReadBool(in, &a->references_parameter_pack);
        break;
      case kTArg_type:
        a->type = (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kTArg_int_value:
        WireReadInt64(in, &a->int_value);
        break;
      case kTArg_template_parameter_index:
        WireReadInt32(in, &a->template_parameter_index);
        break;
      case kTArg_pack_arguments:
        a->pack_arguments = ReadTemplateArgumentVector(ctx, in);
        break;
      case kTArg_dependent_expr:
        a->dependent_expr = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kTArg_location: {
        uint64_t loc = 0;
        WireReadUint64(in, &loc);
        a->location = (SourceLocation)loc;
        break;
      }
      case kTArg_value_kind: {
        int32_t value;
        WireReadInt32(in, &value);
        a->value_kind = (TemplateValueKind)value;
        break;
      }
      case kTArg_value_symbol:
        a->value_symbol = (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_value_offset:
        WireReadInt64(in, &a->value_offset);
        break;
      case kTArg_value_adjustment:
        WireReadInt64(in, &a->value_adjustment);
        break;
      case kTArg_member_function:
        a->member_function =
            (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_template_symbol:
        a->template_symbol =
            (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_object_initializer:
        a->object_initializer =
            (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kTArg_reflection_kind: {
        int32_t value = 0;
        WireReadInt32(in, &value);
        a->reflection_value = ReflectionCreateDeserialized(
            (ReflectionEntityKind)value, a->location);
        break;
      }
      case kTArg_reflection_type:
        if (a->reflection_value != NULL) {
          a->reflection_value->reflected_type =
              (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        } else {
          (void)SReadRef(ctx, in, kSerialKindType);
        }
        break;
      case kTArg_reflection_symbol:
        if (a->reflection_value != NULL) {
          a->reflection_value->symbol =
              (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        } else {
          (void)SReadRef(ctx, in, kSerialKindSymbol);
        }
        break;
      case kTArg_reflection_member:
        if (a->reflection_value != NULL) {
          a->reflection_value->member =
              (StructMember*)SReadRef(ctx, in, kSerialKindStructMember);
        } else {
          (void)SReadRef(ctx, in, kSerialKindStructMember);
        }
        break;
      case kTArg_reflection_namespace:
        if (a->reflection_value != NULL) {
          a->reflection_value->namespace_ =
              (Namespace*)SReadRef(ctx, in, kSerialKindNamespace);
        } else {
          (void)SReadRef(ctx, in, kSerialKindNamespace);
        }
        break;
      case kTArg_reflection_parent:
        if (a->reflection_value != NULL) {
          a->reflection_value->parent_class =
              (Struct*)SReadRef(ctx, in, kSerialKindStruct);
        } else {
          (void)SReadRef(ctx, in, kSerialKindStruct);
        }
        break;
      case kTArg_reflection_base_index: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        if (a->reflection_value != NULL) {
          a->reflection_value->base_index = (size_t)value;
        }
        break;
      }
      case kTArg_reflection_location: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        if (a->reflection_value != NULL) {
          a->reflection_value->location = (SourceLocation)value;
        }
        break;
      }
      case kTArg_reflection_parameter_index: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        if (a->reflection_value != NULL) {
          a->reflection_value->parameter_index = (size_t)value;
        }
        break;
      }
      case kTArg_reflection_scalar_ivalue:
        if (a->reflection_value != NULL) {
          WireReadInt64(in, &a->reflection_value->scalar_ivalue);
        } else {
          int64_t ignored = 0;
          WireReadInt64(in, &ignored);
        }
        break;
      case kTArg_reflection_scalar_fvalue:
        if (a->reflection_value != NULL) {
          WireReadDouble(in, &a->reflection_value->scalar_fvalue);
        } else {
          double ignored = 0.0;
          WireReadDouble(in, &ignored);
        }
        break;
      case kTArg_reflection_scalar_is_float:
        if (a->reflection_value != NULL) {
          WireReadBool(in, &a->reflection_value->scalar_is_float);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_namespace_alias_target:
        if (a->reflection_value != NULL) {
          a->reflection_value->namespace_alias_target =
              (Namespace*)SReadRef(ctx, in, kSerialKindNamespace);
        } else {
          (void)SReadRef(ctx, in, kSerialKindNamespace);
        }
        break;
      case kTArg_reflection_constexpr_initializer:
        if (a->reflection_value != NULL) {
          a->reflection_value->constexpr_initializer =
              (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        } else {
          (void)SReadRef(ctx, in, kSerialKindAST);
        }
        break;
      case kTArg_reflection_promoted_symbol:
        if (a->reflection_value != NULL) {
          a->reflection_value->promoted_symbol =
              (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        } else {
          (void)SReadRef(ctx, in, kSerialKindSymbol);
        }
        break;
      case kTArg_reflection_substituted_template:
        if (a->reflection_value != NULL) {
          a->reflection_value->substituted_template =
              (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        } else {
          (void)SReadRef(ctx, in, kSerialKindSymbol);
        }
        break;
      case kTArg_reflection_extract_type:
        if (a->reflection_value != NULL) {
          a->reflection_value->extract_type =
              (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
          TypeRecordIncRef(a->reflection_value->extract_type);
        } else {
          (void)SReadRef(ctx, in, kSerialKindType);
        }
        break;
      case kTArg_reflection_dms_member_type:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          a->reflection_value->data_member_spec->member_type =
              (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
          TypeRecordIncRef(
              a->reflection_value->data_member_spec->member_type);
        } else {
          (void)SReadRef(ctx, in, kSerialKindType);
        }
        break;
      case kTArg_reflection_dms_name:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          SReadStringVal(ctx, in, &a->reflection_value->data_member_spec->name);
        } else {
          String ignored;
          StringInit(&ignored, NULL);
          SReadStringVal(ctx, in, &ignored);
          StringDestruct(&ignored);
        }
        break;
      case kTArg_reflection_dms_alignment:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          uint64_t alignment = 0;
          WireReadUint64(in, &alignment);
          a->reflection_value->data_member_spec->alignment = (size_t)alignment;
        } else {
          uint64_t ignored = 0;
          WireReadUint64(in, &ignored);
        }
        break;
      case kTArg_reflection_dms_bit_width:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          uint64_t bit_width = 0;
          WireReadUint64(in, &bit_width);
          a->reflection_value->data_member_spec->bit_width = (size_t)bit_width;
        } else {
          uint64_t ignored = 0;
          WireReadUint64(in, &ignored);
        }
        break;
      case kTArg_reflection_dms_no_unique_address:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          WireReadBool(in,
                       &a->reflection_value->data_member_spec->no_unique_address);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_dms_has_name:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          WireReadBool(in, &a->reflection_value->data_member_spec->has_name);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_dms_has_alignment:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          WireReadBool(in,
                       &a->reflection_value->data_member_spec->has_alignment);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_dms_has_bit_width:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->data_member_spec == NULL) {
            a->reflection_value->data_member_spec =
                ReflectionDataMemberSpecNew(NULL);
          }
          WireReadBool(in, &a->reflection_value->data_member_spec->has_bit_width);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_ens_name:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->enumerator_spec == NULL) {
            a->reflection_value->enumerator_spec = ReflectionEnumeratorSpecNew();
          }
          SReadStringVal(ctx, in, &a->reflection_value->enumerator_spec->name);
        } else {
          String ignored;
          StringInit(&ignored, NULL);
          SReadStringVal(ctx, in, &ignored);
          StringDestruct(&ignored);
        }
        break;
      case kTArg_reflection_ens_value:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->enumerator_spec == NULL) {
            a->reflection_value->enumerator_spec = ReflectionEnumeratorSpecNew();
          }
          WireReadInt64(in, &a->reflection_value->enumerator_spec->value);
        } else {
          int64_t ignored = 0;
          WireReadInt64(in, &ignored);
        }
        break;
      case kTArg_reflection_ens_has_value:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->enumerator_spec == NULL) {
            a->reflection_value->enumerator_spec = ReflectionEnumeratorSpecNew();
          }
          WireReadBool(in, &a->reflection_value->enumerator_spec->has_value);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      case kTArg_reflection_ens_has_name:
        if (a->reflection_value != NULL) {
          if (a->reflection_value->enumerator_spec == NULL) {
            a->reflection_value->enumerator_spec = ReflectionEnumeratorSpecNew();
          }
          WireReadBool(in, &a->reflection_value->enumerator_spec->has_name);
        } else {
          bool ignored = false;
          WireReadBool(in, &ignored);
        }
        break;
      default:
        if (a->reflection_value != NULL &&
            SerialReadReflectionExtendedField(ctx, in, field,
                                              a->reflection_value)) {
          break;
        }
        WireSkip(in, wt);
        break;
    }
  }
  if (a->kind == kTemplateParameterNonType &&
      a->value_kind == kTemplateValueNone &&
      a->template_parameter_index < 0 && a->dependent_expr == NULL) {
    // Modules written before typed NTTPs only carried int_value.
    a->value_kind = kTemplateValueIntegral;
  }
  a->reflection_value = ReflectionCanonicalize(a->reflection_value);
  return a;
}

static void WriteTemplateArgumentVector(SerializeContext* ctx, WireBuffer* buf,
                                        int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    WriteTemplateArgument(ctx, &elem, (TemplateArgument*)VectorGet(v, i));
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static Vector* ReadTemplateArgumentVector(DeserializeContext* ctx,
                                          WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
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
    VectorAppend(out, ReadTemplateArgument(ctx, &er));
  }
  return out;
}

static void WriteTemplateArgumentVectorVector(SerializeContext* ctx,
                                              WireBuffer* out, int field,
                                              Vector* vectors) {
  WireBuffer payload;
  WireBufferInitOwned(&payload, 16);
  size_t count = vectors != NULL ? vectors->length : 0;
  WireWriteRawVarint(&payload, count);
  for (size_t i = 0; i < count; i++) {
    Vector* arguments = vectors->value.p[i];
    WireWriteRawVarint(&payload, arguments != NULL ? 1 : 0);
    if (arguments == NULL) {
      continue;
    }
    WireBuffer element;
    WireBufferInitOwned(&element, 16);
    WriteTemplateArgumentVector(ctx, &element, 1, arguments);
    WireWriteRawVarint(&payload, WireBufferSize(&element));
    WireWriteRaw(&payload, WireBufferData(&element),
                 WireBufferSize(&element));
    WireBufferDestruct(&element);
  }
  WireWriteBytes(out, field, WireBufferData(&payload),
                 WireBufferSize(&payload));
  WireBufferDestruct(&payload);
}

static Vector* ReadTemplateArgumentVectorVector(DeserializeContext* ctx,
                                                WireBuffer* in) {
  const void* data;
  size_t length;
  if (!WireReadBytes(in, &data, &length)) {
    return NULL;
  }
  WireBuffer payload;
  WireBufferInitReader(&payload, data, length);
  uint64_t count;
  if (!WireReadRawVarint(&payload, &count)) {
    return NULL;
  }
  Vector* vectors = NewVector();
  for (uint64_t i = 0; i < count; i++) {
    uint64_t present;
    if (!WireReadRawVarint(&payload, &present)) {
      break;
    }
    if (present == 0) {
      VectorAppend(vectors, NULL);
      continue;
    }
    const void* element;
    size_t element_length;
    if (!WireReadBytes(&payload, &element, &element_length)) {
      break;
    }
    WireBuffer reader;
    WireBufferInitReader(&reader, element, element_length);
    int element_field;
    WireType wire_type;
    if (!WireReadTag(&reader, &element_field, &wire_type) ||
        element_field != 1 || wire_type != kWireLengthDelimited) {
      VectorAppend(vectors, NULL);
      continue;
    }
    VectorAppend(vectors, ReadTemplateArgumentVector(ctx, &reader));
  }
  return vectors;
}

// Public wrappers so constraint_serialize.c can (de)serialize the
// TemplateArgument* vectors held by concept-id constraints.
void SerialWriteTemplateArgumentVector(SerializeContext* ctx, WireBuffer* buf,
                                       int field, Vector* v) {
  WriteTemplateArgumentVector(ctx, buf, field, v);
}

Vector* SerialReadTemplateArgumentVector(DeserializeContext* ctx,
                                         WireBuffer* in) {
  return ReadTemplateArgumentVector(ctx, in);
}

// ---------------------------------------------------------------------------
// ArrayInfo (inline).
// ---------------------------------------------------------------------------
static void WriteArrayInfo(SerializeContext* ctx, WireBuffer* out,
                           ArrayInfo* a) {
  WireWriteBool(out, kArr_is_flexible, a->is_flexible);
  WireWriteBool(out, kArr_is_static, a->is_static);
  WireWriteBool(out, kArr_is_vla, a->is_vla);
  WireWriteBool(out, kArr_is_placeholder_vla, a->is_placeholder_vla);
  WireWriteBool(out, kArr_is_dependent_bound, a->is_dependent_bound);
  WireWriteInt32(out, kArr_template_parameter_index,
                 a->template_parameter_index);
  if (a->is_vla || a->is_dependent_bound) {
    SWriteRef(ctx, out, kArr_vla_size, kSerialKindAST, a->size.vla.size);
  } else {
    WireWriteInt32(out, kArr_fixed, a->size.fixed);
  }
}

static void ReadArrayInfo(DeserializeContext* ctx, WireBuffer* in,
                          ArrayInfo* a) {
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    bool b;
    switch (field) {
      case kArr_is_flexible:
        WireReadBool(in, &b);
        a->is_flexible = b;
        break;
      case kArr_is_static:
        WireReadBool(in, &b);
        a->is_static = b;
        break;
      case kArr_is_vla:
        WireReadBool(in, &b);
        a->is_vla = b;
        break;
      case kArr_is_placeholder_vla:
        WireReadBool(in, &b);
        a->is_placeholder_vla = b;
        break;
      case kArr_is_dependent_bound:
        WireReadBool(in, &b);
        a->is_dependent_bound = b;
        break;
      case kArr_template_parameter_index:
        WireReadInt32(in, &a->template_parameter_index);
        break;
      case kArr_fixed:
        WireReadInt32(in, &a->size.fixed);
        break;
      case kArr_vla_size:
        a->size.vla.size = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      default:
        WireSkip(in, wt);
        break;
    }
  }
}

// ---------------------------------------------------------------------------
// FunctionInfo (inline).
// ---------------------------------------------------------------------------
static void WriteContractAssertionVector(SerializeContext* ctx,
                                         WireBuffer* out, int field,
                                         Vector* assertions) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 32);
  WireWriteRawVarint(&tmp, (uint64_t)assertions->length);
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    WireBuffer elem;
    WireBufferInitOwned(&elem, 32);
    WireWriteInt32(&elem, 1, (int32_t)assertion->kind);
    SWriteRef(ctx, &elem, 2, kSerialKindAST, assertion->predicate);
    SWriteRef(ctx, &elem, 3, kSerialKindSymbol,
              assertion->result_binding);
    WireWriteUint64(&elem, 4, (uint64_t)assertion->location);
    SerialWriteAttributeVector(ctx, &elem, 5, &assertion->attributes);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(out, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadContractAssertionVector(DeserializeContext* ctx,
                                        WireBuffer* in, Vector* assertions) {
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
    const void* elem_data;
    size_t elem_len;
    if (!WireReadBytes(&sub, &elem_data, &elem_len)) {
      return;
    }
    WireBuffer elem;
    WireBufferInitReader(&elem, elem_data, elem_len);
    ContractAssertionKind kind = kContractPrecondition;
    ASTNode* predicate = NULL;
    Symbol* result_binding = NULL;
    SourceLocation location = SOURCE_LOCATION_MISSING;
    Vector attrs = {0};
    VectorInit(&attrs);
    while (!WireBufferEof(&elem) && !WireBufferHasError(&elem)) {
      int elem_field;
      WireType wt;
      if (!WireReadTag(&elem, &elem_field, &wt)) {
        break;
      }
      if (elem_field == 1) {
        int32_t value;
        WireReadInt32(&elem, &value);
        kind = (ContractAssertionKind)value;
      } else if (elem_field == 2) {
        predicate = (ASTNode*)SReadRef(ctx, &elem, kSerialKindAST);
      } else if (elem_field == 3) {
        result_binding =
            (Symbol*)SReadRef(ctx, &elem, kSerialKindSymbol);
      } else if (elem_field == 4) {
        uint64_t value;
        WireReadUint64(&elem, &value);
        location = (SourceLocation)value;
      } else if (elem_field == 5) {
        SerialReadAttributeVector(ctx, &elem, &attrs);
      } else {
        WireSkip(&elem, wt);
      }
    }
    VectorAppend(assertions,
                 NewContractAssertion(kind, predicate, result_binding,
                                      &attrs, location));
  }
}

static void WriteFunctionInfo(SerializeContext* ctx, WireBuffer* out,
                              FunctionInfo* f) {
  SWriteRef(ctx, out, kFn_symbol, kSerialKindSymbol, f->symbol);
  SWriteRefVector(ctx, out, kFn_prototype, kSerialKindSymbol, &f->prototype);
  WireWriteBool(out, kFn_varargs, f->varargs);
  bool body_is_reachable =
      !ctx->writing_module_interface || f->is_inline || f->is_constexpr ||
      f->template_parameter_count > 0 ||
      (f->symbol != NULL && f->symbol->flags.is_template);
  if (body_is_reachable) {
    SWriteRef(ctx, out, kFn_body, kSerialKindAST, f->body);
  }
  SWriteRef(ctx, out, kFn_explicit_condition, kSerialKindAST,
            f->explicit_condition);
  WireWriteBool(out, kFn_unknown_args, f->unknown_args);
  WireWriteBool(out, kFn_definition, f->definition);
  WireWriteBool(out, kFn_old_style, f->old_style);
  WireWriteBool(out, kFn_is_inline, f->is_inline);
  WireWriteBool(out, kFn_is_constexpr, f->is_constexpr);
  WireWriteBool(out, kFn_is_consteval, f->is_consteval);
  WireWriteBool(out, kFn_is_constructor, f->is_constructor);
  WireWriteBool(out, kFn_is_destructor, f->is_destructor);
  WireWriteBool(out, kFn_is_const_member, f->is_const_member);
  WireWriteBool(out, kFn_is_volatile_member, f->is_volatile_member);
  WireWriteBool(out, kFn_has_explicit_object_parameter,
                f->has_explicit_object_parameter);
  WireWriteInt32(out, kFn_ref_qualifier, (int32_t)f->ref_qualifier);
  WireWriteBool(out, kFn_is_explicit, f->is_explicit);
  WireWriteBool(out, kFn_is_explicit_conversion, f->is_explicit_conversion);
  WireWriteBool(out, kFn_is_virtual, f->is_virtual);
  WireWriteBool(out, kFn_is_override, f->is_override);
  WireWriteBool(out, kFn_is_final, f->is_final);
  WireWriteBool(out, kFn_is_pure_virtual, f->is_pure_virtual);
  WireWriteBool(out, kFn_is_defaulted, f->is_defaulted);
  WireWriteBool(out, kFn_is_deleted, f->is_deleted);
  WireWriteInt32(out, kFn_cxx_special_member_kind,
                 (int32_t)f->cxx_special_member_kind);
  WireWriteBool(out, kFn_is_user_declared, f->is_user_declared);
  WireWriteBool(out, kFn_is_user_provided, f->is_user_provided);
  WireWriteBool(out, kFn_is_explicitly_defaulted, f->is_explicitly_defaulted);
  WireWriteBool(out, kFn_is_explicitly_deleted, f->is_explicitly_deleted);
  WireWriteBool(out, kFn_is_implicitly_declared, f->is_implicitly_declared);
  WireWriteBool(out, kFn_is_implicitly_deleted, f->is_implicitly_deleted);
  WireWriteBool(out, kFn_is_trivial_special_member,
                f->is_trivial_special_member);
  WireWriteBool(out, kFn_is_constexpr_eligible, f->is_constexpr_eligible);
  WireWriteBool(out, kFn_is_noexcept_eligible, f->is_noexcept_eligible);
  WireWriteBool(out, kFn_is_noexcept, f->is_noexcept);
  WireWriteBool(out, kFn_is_auto_return_deduced, f->is_auto_return_deduced);
  WireWriteBool(out, kFn_is_decltype_auto_return_deduced,
                f->is_decltype_auto_return_deduced);
  SWriteStringPtr(ctx, out, kFn_deleted_reason, f->deleted_reason);
  WriteContractAssertionVector(ctx, out, kFn_contract_assertions,
                               &f->contract_assertions);
  WireWriteBool(out, kFn_is_deduction_guide, f->is_deduction_guide);
  WireWriteBool(out, kFn_is_coroutine, f->is_coroutine);
  SWriteRef(ctx, out, kFn_coroutine_promise_type, kSerialKindType,
            f->coroutine_promise_type);
  SWriteRef(ctx, out, kFn_coroutine_frame_type, kSerialKindType,
            f->coroutine_frame_type);
  WireWriteInt32(out, kFn_coroutine_suspend_count, f->coroutine_suspend_count);
  WireWriteInt32(out, kFn_virtual_index, f->virtual_index);
  SWriteRef(ctx, out, kFn_cxx_member_owner, kSerialKindStruct,
            f->cxx_member_owner);
  SWriteRef(ctx, out, kFn_template_origin, kSerialKindSymbol,
            f->template_origin);
  WireWriteInt32(out, kFn_template_parameter_count,
                 f->template_parameter_count);
  WireWriteInt32(out, kFn_template_parameter_base, f->template_parameter_base);
  SerialWriteTemplateParameterVector(ctx, out, kFn_template_parameters,
                                     &f->template_parameters);
  SWriteRefVector(ctx, out, kFn_template_instantiations, kSerialKindSymbol,
                  &f->template_instantiations);
  SerialWriteConstraint(ctx, out, kFn_associated_constraint,
                        f->associated_constraint);
}

static void ReadFunctionInfo(DeserializeContext* ctx, WireBuffer* in,
                             FunctionInfo* f) {
  VectorInit(&f->prototype);
  VectorInit(&f->template_parameters);
  VectorInit(&f->template_instantiations);
  VectorInit(&f->contract_assertions);
  f->virtual_index = -1;
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kFn_symbol:
        f->symbol = (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kFn_prototype:
        SReadRefVector(ctx, in, kSerialKindSymbol, &f->prototype);
        break;
      case kFn_varargs:
        WireReadBool(in, &f->varargs);
        break;
      case kFn_body:
        f->body = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kFn_explicit_condition:
        f->explicit_condition = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kFn_unknown_args:
        WireReadBool(in, &f->unknown_args);
        break;
      case kFn_definition:
        WireReadBool(in, &f->definition);
        break;
      case kFn_old_style:
        WireReadBool(in, &f->old_style);
        break;
      case kFn_is_inline:
        WireReadBool(in, &f->is_inline);
        break;
      case kFn_is_constexpr:
        WireReadBool(in, &f->is_constexpr);
        break;
      case kFn_is_consteval:
        WireReadBool(in, &f->is_consteval);
        break;
      case kFn_is_constructor:
        WireReadBool(in, &f->is_constructor);
        break;
      case kFn_is_destructor:
        WireReadBool(in, &f->is_destructor);
        break;
      case kFn_is_const_member:
        WireReadBool(in, &f->is_const_member);
        break;
      case kFn_is_volatile_member:
        WireReadBool(in, &f->is_volatile_member);
        break;
      case kFn_has_explicit_object_parameter:
        WireReadBool(in, &f->has_explicit_object_parameter);
        break;
      case kFn_ref_qualifier: {
        int32_t v;
        WireReadInt32(in, &v);
        f->ref_qualifier = (CXXRefQualifier)v;
        break;
      }
      case kFn_is_explicit:
        WireReadBool(in, &f->is_explicit);
        break;
      case kFn_is_explicit_conversion:
        WireReadBool(in, &f->is_explicit_conversion);
        break;
      case kFn_is_virtual:
        WireReadBool(in, &f->is_virtual);
        break;
      case kFn_is_override:
        WireReadBool(in, &f->is_override);
        break;
      case kFn_is_final:
        WireReadBool(in, &f->is_final);
        break;
      case kFn_is_pure_virtual:
        WireReadBool(in, &f->is_pure_virtual);
        break;
      case kFn_is_defaulted:
        WireReadBool(in, &f->is_defaulted);
        break;
      case kFn_is_deleted:
        WireReadBool(in, &f->is_deleted);
        break;
      case kFn_cxx_special_member_kind: {
        int32_t v;
        WireReadInt32(in, &v);
        f->cxx_special_member_kind = (CXXSpecialMemberKind)v;
        break;
      }
      case kFn_is_user_declared:
        WireReadBool(in, &f->is_user_declared);
        break;
      case kFn_is_user_provided:
        WireReadBool(in, &f->is_user_provided);
        break;
      case kFn_is_explicitly_defaulted:
        WireReadBool(in, &f->is_explicitly_defaulted);
        break;
      case kFn_is_explicitly_deleted:
        WireReadBool(in, &f->is_explicitly_deleted);
        break;
      case kFn_is_implicitly_declared:
        WireReadBool(in, &f->is_implicitly_declared);
        break;
      case kFn_is_implicitly_deleted:
        WireReadBool(in, &f->is_implicitly_deleted);
        break;
      case kFn_is_trivial_special_member:
        WireReadBool(in, &f->is_trivial_special_member);
        break;
      case kFn_is_constexpr_eligible:
        WireReadBool(in, &f->is_constexpr_eligible);
        break;
      case kFn_is_noexcept_eligible:
        WireReadBool(in, &f->is_noexcept_eligible);
        break;
      case kFn_is_noexcept:
        WireReadBool(in, &f->is_noexcept);
        break;
      case kFn_is_auto_return_deduced:
        WireReadBool(in, &f->is_auto_return_deduced);
        break;
      case kFn_is_decltype_auto_return_deduced:
        WireReadBool(in, &f->is_decltype_auto_return_deduced);
        break;
      case kFn_deleted_reason:
        f->deleted_reason = SReadStringPtr(ctx, in);
        break;
      case kFn_contract_assertions:
        ReadContractAssertionVector(ctx, in, &f->contract_assertions);
        break;
      case kFn_is_deduction_guide:
        WireReadBool(in, &f->is_deduction_guide);
        break;
      case kFn_is_coroutine:
        WireReadBool(in, &f->is_coroutine);
        break;
      case kFn_coroutine_promise_type:
        f->coroutine_promise_type =
            (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kFn_coroutine_frame_type:
        f->coroutine_frame_type =
            (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kFn_coroutine_suspend_count:
        WireReadInt32(in, &f->coroutine_suspend_count);
        break;
      case kFn_virtual_index:
        WireReadInt32(in, &f->virtual_index);
        break;
      case kFn_cxx_member_owner:
        f->cxx_member_owner = (Struct*)SReadRef(ctx, in, kSerialKindStruct);
        break;
      case kFn_template_origin:
        f->template_origin = (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kFn_template_parameter_count:
        WireReadInt32(in, &f->template_parameter_count);
        break;
      case kFn_template_parameter_base:
        WireReadInt32(in, &f->template_parameter_base);
        break;
      case kFn_template_parameters:
        SerialReadTemplateParameterVector(ctx, in, &f->template_parameters);
        break;
      case kFn_template_instantiations:
        SReadRefVector(ctx, in, kSerialKindSymbol, &f->template_instantiations);
        break;
      case kFn_associated_constraint:
        f->associated_constraint = SerialReadConstraint(ctx, in);
        break;
      default:
        WireSkip(in, wt);
        break;
    }
  }
}

// ---------------------------------------------------------------------------
// CXXBaseSpecifier vector (inline).
// ---------------------------------------------------------------------------
static void WriteBaseVector(SerializeContext* ctx, WireBuffer* buf, int field,
                            Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    CXXBaseSpecifier* b = (CXXBaseSpecifier*)VectorGet(v, i);
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    SWriteRef(ctx, &elem, kBase_type, kSerialKindType, b->type);
    WireWriteInt32(&elem, kBase_access, (int32_t)b->access);
    WireWriteInt32(&elem, kBase_byte_offset, b->byte_offset);
    WireWriteBool(&elem, kBase_is_virtual, b->is_virtual);
    WireWriteBool(&elem, kBase_is_pack_expansion, b->is_pack_expansion);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadBaseVector(DeserializeContext* ctx, WireBuffer* in,
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
    CXXBaseSpecifier* b = (CXXBaseSpecifier*)calloc(1, sizeof(*b));
    while (!WireBufferEof(&er) && !WireBufferHasError(&er)) {
      int field;
      WireType wt;
      if (!WireReadTag(&er, &field, &wt)) {
        break;
      }
      switch (field) {
        case kBase_type:
          b->type = (TypeRecord*)SReadRef(ctx, &er, kSerialKindType);
          break;
        case kBase_access: {
          int32_t v;
          WireReadInt32(&er, &v);
          b->access = (CXXAccess)v;
          break;
        }
        case kBase_byte_offset:
          WireReadInt32(&er, &b->byte_offset);
          break;
        case kBase_is_virtual:
          WireReadBool(&er, &b->is_virtual);
          break;
        case kBase_is_pack_expansion:
          WireReadBool(&er, &b->is_pack_expansion);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    VectorAppend(out, b);
  }
}

// ---------------------------------------------------------------------------
// CXXMemberUsingDeclaration vector (inline).
// ---------------------------------------------------------------------------
static void WriteMemberUsingVector(SerializeContext* ctx, WireBuffer* buf,
                                   int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    CXXMemberUsingDeclaration* decl =
        (CXXMemberUsingDeclaration*)VectorGet(v, i);
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    SWriteRef(ctx, &elem, kMemberUsing_base_type, kSerialKindType,
              decl->base_type);
    SWriteStringVal(ctx, &elem, kMemberUsing_member_name, &decl->member_name);
    WireWriteInt32(&elem, kMemberUsing_access, (int32_t)decl->access);
    WireWriteUint64(&elem, kMemberUsing_location,
                    (uint64_t)decl->location);
    WireWriteBool(&elem, kMemberUsing_is_pack_expansion,
                  decl->is_pack_expansion);
    WireWriteBool(&elem, kMemberUsing_qualifier_names_constructor,
                  decl->qualifier_names_constructor);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadMemberUsingVector(DeserializeContext* ctx, WireBuffer* in,
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
    CXXMemberUsingDeclaration* decl =
        (CXXMemberUsingDeclaration*)calloc(1, sizeof(*decl));
    StringInit(&decl->member_name, NULL);
    while (!WireBufferEof(&er) && !WireBufferHasError(&er)) {
      int field;
      WireType wt;
      if (!WireReadTag(&er, &field, &wt)) {
        break;
      }
      switch (field) {
        case kMemberUsing_base_type:
          decl->base_type =
              (TypeRecord*)SReadRef(ctx, &er, kSerialKindType);
          break;
        case kMemberUsing_member_name:
          SReadStringVal(ctx, &er, &decl->member_name);
          break;
        case kMemberUsing_access: {
          int32_t value;
          WireReadInt32(&er, &value);
          decl->access = (CXXAccess)value;
          break;
        }
        case kMemberUsing_location: {
          uint64_t value;
          WireReadUint64(&er, &value);
          decl->location = (SourceLocation)value;
          break;
        }
        case kMemberUsing_is_pack_expansion:
          WireReadBool(&er, &decl->is_pack_expansion);
          break;
        case kMemberUsing_qualifier_names_constructor:
          WireReadBool(&er, &decl->qualifier_names_constructor);
          break;
        default:
          WireSkip(&er, wt);
          break;
      }
    }
    VectorAppend(out, decl);
  }
}

// ---------------------------------------------------------------------------
// CXXFriendTypeDeclaration vector (inline).
// ---------------------------------------------------------------------------
static void WriteFriendTypeVector(SerializeContext* ctx, WireBuffer* buf,
                                  int field, Vector* declarations) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = declarations == NULL ? 0 : declarations->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    CXXFriendTypeDeclaration* declaration =
        declarations->value.p[i];
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    SWriteRef(ctx, &elem, kFriendType_type, kSerialKindType,
              declaration->type);
    WireWriteUint64(&elem, kFriendType_location,
                    (uint64_t)declaration->location);
    WireWriteBool(&elem, kFriendType_is_pack_expansion,
                  declaration->is_pack_expansion);
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static void ReadFriendTypeVector(DeserializeContext* ctx, WireBuffer* in,
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
    size_t elem_len;
    if (!WireReadBytes(&sub, &elem, &elem_len)) {
      return;
    }
    WireBuffer reader;
    WireBufferInitReader(&reader, elem, elem_len);
    CXXFriendTypeDeclaration* declaration =
        calloc(1, sizeof(CXXFriendTypeDeclaration));
    while (!WireBufferEof(&reader) && !WireBufferHasError(&reader)) {
      int field;
      WireType wire_type;
      if (!WireReadTag(&reader, &field, &wire_type)) {
        break;
      }
      switch (field) {
        case kFriendType_type:
          declaration->type =
              (TypeRecord*)SReadRef(ctx, &reader, kSerialKindType);
          break;
        case kFriendType_location: {
          uint64_t location;
          WireReadUint64(&reader, &location);
          declaration->location = (SourceLocation)location;
          break;
        }
        case kFriendType_is_pack_expansion:
          WireReadBool(&reader, &declaration->is_pack_expansion);
          break;
        default:
          WireSkip(&reader, wire_type);
          break;
      }
    }
    VectorAppend(out, declaration);
  }
}

// ---------------------------------------------------------------------------
// TypeRecord.
// ---------------------------------------------------------------------------
static bool WriteType(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  TypeRecord* t = (TypeRecord*)obj;
  assert(t->declarator != kDeclArray || t->next != NULL);
  WireWriteInt32(buf, kType_id, t->id);
  WireWriteInt32(buf, kType_type, (int32_t)t->type);
  WireWriteInt32(buf, kType_qualifiers, (int32_t)t->qualifiers);
  WireWriteInt32(buf, kType_declarator, (int32_t)t->declarator);
  WireWriteInt32(buf, kType_size, t->size);
  WireWriteInt32(buf, kType_bit_width, t->bit_width);
  WireWriteInt32(buf, kType_template_parameter_index,
                 t->template_parameter_index);
  SWriteStringPtr(ctx, buf, kType_template_parameter_name,
                  t->template_parameter_name);
  SWriteStringPtr(ctx, buf, kType_dependent_member_name,
                  t->dependent_member_name);
  SWriteRef(ctx, buf, kType_template_origin, kSerialKindSymbol,
            t->template_origin);
  if (t->template_arguments != NULL) {
    WriteTemplateArgumentVector(ctx, buf, kType_template_arguments,
                                t->template_arguments);
  }
  if (t->dependent_member_template_arguments != NULL) {
    WriteTemplateArgumentVectorVector(
        ctx, buf, kType_dependent_member_template_arguments,
        t->dependent_member_template_arguments);
  }
  SWriteRef(ctx, buf, kType_dependent_decltype_expr, kSerialKindAST,
            t->dependent_decltype_expr);
  WireWriteBool(buf, kType_is_pack_index, t->is_pack_index);
  SWriteRef(ctx, buf, kType_pack_index_expr, kSerialKindAST,
            t->pack_index_expr);
  SWriteRef(ctx, buf, kType_dependent_splice_expr, kSerialKindAST,
            t->dependent_splice_expr);
  SWriteRef(ctx, buf, kType_next, kSerialKindType, t->next);

  if (t->declarator == kDeclArray) {
    WireBuffer sub;
    WireBufferInitOwned(&sub, 16);
    WriteArrayInfo(ctx, &sub, &t->info.array);
    WireWriteBytes(buf, kType_array, WireBufferData(&sub), WireBufferSize(&sub));
    WireBufferDestruct(&sub);
  } else if (t->declarator == kDeclFunction) {
    WireBuffer sub;
    WireBufferInitOwned(&sub, 32);
    WriteFunctionInfo(ctx, &sub, &t->info.function);
    WireWriteBytes(buf, kType_function, WireBufferData(&sub),
                   WireBufferSize(&sub));
    WireBufferDestruct(&sub);
  } else if ((t->type & (kTypeStruct | kTypeUnion)) != 0) {
    SWriteRef(ctx, buf, kType_struct_info, kSerialKindStruct,
              t->info.struct_info);
  } else if ((t->type & kTypeEnum) != 0) {
    SWriteRef(ctx, buf, kType_enum_info, kSerialKindEnum, t->info.enum_info);
  }
  return !WireBufferHasError(buf);
}

static void* AllocType(DeserializeContext* ctx, const void* blob, size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewTypeRecord(0, kQualPlain);
}

static bool ReadType(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  TypeRecord* t = (TypeRecord*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kType_id:
        WireReadInt32(buf, &t->id);
        break;
      case kType_type: {
        int32_t v;
        WireReadInt32(buf, &v);
        t->type = (Type)v;
        break;
      }
      case kType_qualifiers: {
        int32_t v;
        WireReadInt32(buf, &v);
        t->qualifiers = (Qualifiers)v;
        break;
      }
      case kType_declarator: {
        int32_t v;
        WireReadInt32(buf, &v);
        t->declarator = (Declarator)v;
        break;
      }
      case kType_size:
        WireReadInt32(buf, &t->size);
        break;
      case kType_bit_width:
        WireReadInt32(buf, &t->bit_width);
        break;
      case kType_template_parameter_index:
        WireReadInt32(buf, &t->template_parameter_index);
        break;
      case kType_template_parameter_name:
        t->template_parameter_name = SReadStringPtr(ctx, buf);
        break;
      case kType_dependent_member_name:
        t->dependent_member_name = SReadStringPtr(ctx, buf);
        break;
      case kType_template_origin:
        t->template_origin = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kType_template_arguments:
        t->template_arguments = ReadTemplateArgumentVector(ctx, buf);
        break;
      case kType_dependent_member_template_arguments:
        t->dependent_member_template_arguments =
            ReadTemplateArgumentVectorVector(ctx, buf);
        break;
      case kType_dependent_decltype_expr:
        t->dependent_decltype_expr =
            (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kType_is_pack_index:
        WireReadBool(buf, &t->is_pack_index);
        break;
      case kType_pack_index_expr:
        t->pack_index_expr =
            (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kType_dependent_splice_expr:
        t->dependent_splice_expr =
            (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kType_next:
        t->next = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        // TypeRecordCopy shares the next spine and later releases that shared
        // edge while substituting a specialization.  Preserve the imported
        // owner's reference so the primary template's type is not dismantled
        // after its first instantiation.
        TypeRecordIncRef(t->next);
        break;
      case kType_array: {
        const void* data;
        size_t dlen;
        if (WireReadBytes(buf, &data, &dlen)) {
          WireBuffer sub;
          WireBufferInitReader(&sub, data, dlen);
          ReadArrayInfo(ctx, &sub, &t->info.array);
        }
        break;
      }
      case kType_function: {
        const void* data;
        size_t dlen;
        if (WireReadBytes(buf, &data, &dlen)) {
          WireBuffer sub;
          WireBufferInitReader(&sub, data, dlen);
          ReadFunctionInfo(ctx, &sub, &t->info.function);
        }
        break;
      }
      case kType_struct_info:
        t->info.struct_info = (Struct*)SReadRef(ctx, buf, kSerialKindStruct);
        break;
      case kType_enum_info:
        t->info.enum_info = (Enum*)SReadRef(ctx, buf, kSerialKindEnum);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

// ---------------------------------------------------------------------------
// Enum.
// ---------------------------------------------------------------------------
static bool WriteEnum(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  Enum* e = (Enum*)obj;
  SWriteStringPtr(ctx, buf, kEnum_tag_name, e->tag_name);
  SWriteRef(ctx, buf, kEnum_tag_symbol, kSerialKindSymbol, e->tag_symbol);
  SWriteRefVector(ctx, buf, kEnum_constants, kSerialKindSymbol, &e->constants);
  WireWriteInt64(buf, kEnum_next_value, e->next_value);
  WireWriteBool(buf, kEnum_is_scoped, e->is_scoped);
  WireWriteBool(buf, kEnum_has_fixed_underlying, e->has_fixed_underlying);
  WireWriteInt32(buf, kEnum_fixed_underlying_type,
                 (int32_t)e->fixed_underlying_type);
  WireWriteInt32(buf, kEnum_fixed_underlying_size, e->fixed_underlying_size);
  WireWriteInt32(buf, kEnum_fixed_underlying_bit_width,
                 e->fixed_underlying_bit_width);
  return !WireBufferHasError(buf);
}

static void* AllocEnum(DeserializeContext* ctx, const void* blob, size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewEnum();
}

static bool ReadEnum(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  Enum* e = (Enum*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kEnum_tag_name:
        e->tag_name = SReadStringPtr(ctx, buf);
        break;
      case kEnum_tag_symbol:
        e->tag_symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kEnum_constants:
        SReadRefVector(ctx, buf, kSerialKindSymbol, &e->constants);
        break;
      case kEnum_next_value:
        WireReadInt64(buf, &e->next_value);
        break;
      case kEnum_is_scoped:
        WireReadBool(buf, &e->is_scoped);
        break;
      case kEnum_has_fixed_underlying:
        WireReadBool(buf, &e->has_fixed_underlying);
        break;
      case kEnum_fixed_underlying_type: {
        int32_t v;
        WireReadInt32(buf, &v);
        e->fixed_underlying_type = (Type)v;
        break;
      }
      case kEnum_fixed_underlying_size:
        WireReadInt32(buf, &e->fixed_underlying_size);
        break;
      case kEnum_fixed_underlying_bit_width:
        WireReadInt32(buf, &e->fixed_underlying_bit_width);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

// ---------------------------------------------------------------------------
// StructMember.
// ---------------------------------------------------------------------------
static bool WriteMember(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  StructMember* m = (StructMember*)obj;
  SWriteRef(ctx, buf, kMem_symbol, kSerialKindSymbol, m->symbol);
  SWriteRef(ctx, buf, kMem_default_initializer, kSerialKindAST,
            m->default_initializer);
  WireWriteInt32(buf, kMem_byte_offset, m->byte_offset);
  WireWriteInt32(buf, kMem_bit_offset, m->bit_offset);
  WireWriteInt32(buf, kMem_bit_size, m->bit_size);
  WireWriteInt64(buf, kMem_index, (int64_t)m->index);
  WireWriteInt32(buf, kMem_cxx_vcall_offset, m->cxx_vcall_offset);
  WireWriteBool(buf, kMem_is_anon, m->is_anon);
  WireWriteBool(buf, kMem_is_static, m->is_static);
  WireWriteBool(buf, kMem_is_mutable, m->is_mutable);
  WireWriteBool(buf, kMem_is_member_function, m->is_member_function);
  WireWriteBool(buf, kMem_is_using_declaration, m->is_using_declaration);
  WireWriteInt32(buf, kMem_access, (int32_t)m->access);
  SWriteRef(ctx, buf, kMem_overload_next, kSerialKindStructMember,
            m->overload_next);
  return !WireBufferHasError(buf);
}

static void* AllocMember(DeserializeContext* ctx, const void* blob,
                         size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewStructMember(NULL);
}

static bool ReadMember(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  StructMember* m = (StructMember*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kMem_symbol:
        m->symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kMem_default_initializer:
        m->default_initializer = (ASTNode*)SReadRef(ctx, buf, kSerialKindAST);
        break;
      case kMem_byte_offset:
        WireReadInt32(buf, &m->byte_offset);
        break;
      case kMem_bit_offset:
        WireReadInt32(buf, &m->bit_offset);
        break;
      case kMem_bit_size:
        WireReadInt32(buf, &m->bit_size);
        break;
      case kMem_index: {
        int64_t v;
        WireReadInt64(buf, &v);
        m->index = (size_t)v;
        break;
      }
      case kMem_cxx_vcall_offset:
        WireReadInt32(buf, &m->cxx_vcall_offset);
        break;
      case kMem_is_anon:
        WireReadBool(buf, &m->is_anon);
        break;
      case kMem_is_static:
        WireReadBool(buf, &m->is_static);
        break;
      case kMem_is_mutable:
        WireReadBool(buf, &m->is_mutable);
        break;
      case kMem_is_member_function:
        WireReadBool(buf, &m->is_member_function);
        break;
      case kMem_is_using_declaration:
        WireReadBool(buf, &m->is_using_declaration);
        break;
      case kMem_access: {
        int32_t v;
        WireReadInt32(buf, &v);
        m->access = (CXXAccess)v;
        break;
      }
      case kMem_overload_next:
        m->overload_next =
            (StructMember*)SReadRef(ctx, buf, kSerialKindStructMember);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

enum {
  kPartial_tag_symbol = 1,
  kPartial_template_parameters = 2,
  kPartial_pattern_arguments = 3,
  kPartial_associated_constraint = 4,
  kPartial_variable_initializer = 5,
  kPartial_variable_type = 6,
};

void SerialWritePartialSpecializationVector(SerializeContext* ctx,
                                            WireBuffer* out, int field,
                                            Vector* specializations) {
  WireBuffer list;
  WireBufferInitOwned(&list, 32);
  WireWriteRawVarint(&list, (uint64_t)specializations->length);
  for (size_t i = 0; i < specializations->length; i++) {
    ClassTemplatePartialSpecialization* partial =
        (ClassTemplatePartialSpecialization*)VectorGet(specializations, i);
    WireBuffer item;
    WireBufferInitOwned(&item, 64);
    SWriteRef(ctx, &item, kPartial_tag_symbol, kSerialKindSymbol,
              partial->tag_symbol);
    SerialWriteTemplateParameterVector(
        ctx, &item, kPartial_template_parameters,
        &partial->template_parameters);
    WriteTemplateArgumentVector(ctx, &item, kPartial_pattern_arguments,
                                &partial->pattern_arguments);
    SerialWriteConstraint(ctx, &item, kPartial_associated_constraint,
                          partial->associated_constraint);
    SWriteRef(ctx, &item, kPartial_variable_initializer, kSerialKindAST,
              partial->variable_initializer);
    SWriteRef(ctx, &item, kPartial_variable_type, kSerialKindType,
              partial->variable_type);
    WireWriteRawVarint(&list, (uint64_t)WireBufferSize(&item));
    WireWriteRaw(&list, WireBufferData(&item), WireBufferSize(&item));
    WireBufferDestruct(&item);
  }
  WireWriteBytes(out, field, WireBufferData(&list), WireBufferSize(&list));
  WireBufferDestruct(&list);
}

void SerialReadPartialSpecializationVector(DeserializeContext* ctx,
                                           WireBuffer* in, Vector* out) {
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
  for (uint64_t i = 0; i < count; i++) {
    const void* item_data;
    size_t item_length;
    if (!WireReadBytes(&list, &item_data, &item_length)) {
      return;
    }
    ClassTemplatePartialSpecialization* partial =
        NewClassTemplatePartialSpecialization(NULL, NULL, NULL);
    WireBuffer item;
    WireBufferInitReader(&item, item_data, item_length);
    while (!WireBufferEof(&item) && !WireBufferHasError(&item)) {
      int item_field;
      WireType wire_type;
      if (!WireReadTag(&item, &item_field, &wire_type)) {
        break;
      }
      switch (item_field) {
        case kPartial_tag_symbol:
          partial->tag_symbol =
              (Symbol*)SReadRef(ctx, &item, kSerialKindSymbol);
          break;
        case kPartial_template_parameters:
          SerialReadTemplateParameterVector(
              ctx, &item, &partial->template_parameters);
          break;
        case kPartial_pattern_arguments: {
          Vector* arguments = ReadTemplateArgumentVector(ctx, &item);
          if (arguments != NULL) {
            VectorDestruct(&partial->pattern_arguments);
            partial->pattern_arguments = *arguments;
            free(arguments);
          }
          break;
        }
        case kPartial_associated_constraint:
          partial->associated_constraint =
              SerialReadConstraint(ctx, &item);
          break;
        case kPartial_variable_initializer:
          partial->variable_initializer =
              (ASTNode*)SReadRef(ctx, &item, kSerialKindAST);
          break;
        case kPartial_variable_type:
          partial->variable_type =
              (TypeRecord*)SReadRef(ctx, &item, kSerialKindType);
          break;
        default:
          WireSkip(&item, wire_type);
          break;
      }
    }
    VectorAppend(out, partial);
  }
}

// ---------------------------------------------------------------------------
// Struct.
// ---------------------------------------------------------------------------
static bool WriteStruct(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  Struct* s = (Struct*)obj;
  SWriteStringPtr(ctx, buf, kStruct_tag_name, s->tag_name);
  SWriteRef(ctx, buf, kStruct_tag_symbol, kSerialKindSymbol, s->tag_symbol);
  WriteBaseVector(ctx, buf, kStruct_bases, &s->bases);
  SWriteRefVector(ctx, buf, kStruct_members, kSerialKindStructMember,
                  &s->members);
  WireWriteInt32(buf, kStruct_next_offset, s->next_offset);
  WireWriteInt32(buf, kStruct_size, s->size);
  WireWriteInt32(buf, kStruct_non_virtual_size, s->non_virtual_size);
  WireWriteInt32(buf, kStruct_alignment, s->alignment);
  WireWriteBool(buf, kStruct_is_union, s->is_union);
  WireWriteBool(buf, kStruct_is_class, s->is_class);
  WireWriteBool(buf, kStruct_is_final, s->is_final);
  WireWriteBool(buf, kStruct_is_template, s->is_template);
  WireWriteBool(buf, kStruct_is_aggregate, s->is_aggregate);
  WireWriteBool(buf, kStruct_cxx_special_members_complete,
                s->cxx_special_members_complete);
  SerialWriteTemplateParameterVector(ctx, buf, kStruct_template_parameters,
                                     &s->template_parameters);
  WireWriteInt32(buf, kStruct_template_parameter_count,
                 s->template_parameter_count);
  WireWriteBool(buf, kStruct_packed, s->packed);
  WireWriteBool(buf, kStruct_is_abstract, s->is_abstract);
  WireWriteInt32(buf, kStruct_explicit_alignment, s->explicit_alignment);
  WireWriteInt32(buf, kStruct_pack, s->pack);
  WireWriteInt32(buf, kStruct_next_bit_pos, s->next_bit_pos);
  WireWriteInt32(buf, kStruct_current_offset, s->current_offset);
  SWriteRef(ctx, buf, kStruct_vptr_member, kSerialKindStructMember,
            s->vptr_member);
  SWriteRef(ctx, buf, kStruct_vtable_symbol, kSerialKindSymbol,
            s->vtable_symbol);
  SWriteRef(ctx, buf, kStruct_vbptr_member, kSerialKindStructMember,
            s->vbptr_member);
  SWriteRef(ctx, buf, kStruct_vbtable_symbol, kSerialKindSymbol,
            s->vbtable_symbol);
  SWriteRefVector(ctx, buf, kStruct_virtual_members, kSerialKindStructMember,
                  &s->virtual_members);
  SWriteRefVector(ctx, buf, kStruct_friend_classes, kSerialKindStruct,
                  &s->friend_classes);
  SWriteRefVector(ctx, buf, kStruct_friend_functions, kSerialKindSymbol,
                  &s->friend_functions);
  SerialWriteConstraint(ctx, buf, kStruct_associated_constraint,
                        s->associated_constraint);
  SWriteRef(ctx, buf, kStruct_lexical_parent, kSerialKindStruct,
            s->lexical_parent);
  SerialWritePartialSpecializationVector(
      ctx, buf, kStruct_partial_specializations,
      &s->partial_specializations);
  SWriteRefVector(ctx, buf, kStruct_deduction_guides, kSerialKindSymbol,
                  &s->deduction_guides);
  WriteMemberUsingVector(ctx, buf, kStruct_member_using_declarations,
                         &s->member_using_declarations);
  WriteFriendTypeVector(ctx, buf, kStruct_friend_type_declarations,
                        &s->friend_type_declarations);
  WireWriteBool(buf, kStruct_meta_aggregate_complete, s->meta_aggregate_complete);
  return !WireBufferHasError(buf);
}

static void* AllocStruct(DeserializeContext* ctx, const void* blob,
                         size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return NewStruct(false);
}

static bool ReadStruct(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  Struct* s = (Struct*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kStruct_tag_name:
        s->tag_name = SReadStringPtr(ctx, buf);
        break;
      case kStruct_tag_symbol:
        s->tag_symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kStruct_bases:
        ReadBaseVector(ctx, buf, &s->bases);
        break;
      case kStruct_members:
        SReadRefVector(ctx, buf, kSerialKindStructMember, &s->members);
        break;
      case kStruct_next_offset:
        WireReadInt32(buf, &s->next_offset);
        break;
      case kStruct_size:
        WireReadInt32(buf, &s->size);
        break;
      case kStruct_non_virtual_size:
        WireReadInt32(buf, &s->non_virtual_size);
        break;
      case kStruct_alignment:
        WireReadInt32(buf, &s->alignment);
        break;
      case kStruct_is_union:
        WireReadBool(buf, &s->is_union);
        break;
      case kStruct_is_class:
        WireReadBool(buf, &s->is_class);
        break;
      case kStruct_is_final:
        WireReadBool(buf, &s->is_final);
        break;
      case kStruct_is_template:
        WireReadBool(buf, &s->is_template);
        break;
      case kStruct_is_aggregate:
        WireReadBool(buf, &s->is_aggregate);
        break;
      case kStruct_cxx_special_members_complete:
        WireReadBool(buf, &s->cxx_special_members_complete);
        break;
      case kStruct_template_parameters:
        SerialReadTemplateParameterVector(ctx, buf, &s->template_parameters);
        break;
      case kStruct_template_parameter_count:
        WireReadInt32(buf, &s->template_parameter_count);
        break;
      case kStruct_packed:
        WireReadBool(buf, &s->packed);
        break;
      case kStruct_is_abstract:
        WireReadBool(buf, &s->is_abstract);
        break;
      case kStruct_explicit_alignment:
        WireReadInt32(buf, &s->explicit_alignment);
        break;
      case kStruct_pack:
        WireReadInt32(buf, &s->pack);
        break;
      case kStruct_next_bit_pos:
        WireReadInt32(buf, &s->next_bit_pos);
        break;
      case kStruct_current_offset:
        WireReadInt32(buf, &s->current_offset);
        break;
      case kStruct_vptr_member:
        s->vptr_member =
            (StructMember*)SReadRef(ctx, buf, kSerialKindStructMember);
        break;
      case kStruct_vtable_symbol:
        s->vtable_symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kStruct_vbptr_member:
        s->vbptr_member =
            (StructMember*)SReadRef(ctx, buf, kSerialKindStructMember);
        break;
      case kStruct_vbtable_symbol:
        s->vbtable_symbol = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kStruct_virtual_members:
        SReadRefVector(ctx, buf, kSerialKindStructMember, &s->virtual_members);
        break;
      case kStruct_friend_classes:
        SReadRefVector(ctx, buf, kSerialKindStruct, &s->friend_classes);
        break;
      case kStruct_friend_functions:
        SReadRefVector(ctx, buf, kSerialKindSymbol, &s->friend_functions);
        break;
      case kStruct_associated_constraint:
        s->associated_constraint = SerialReadConstraint(ctx, buf);
        break;
      case kStruct_lexical_parent:
        s->lexical_parent =
            (Struct*)SReadRef(ctx, buf, kSerialKindStruct);
        break;
      case kStruct_partial_specializations:
        SerialReadPartialSpecializationVector(
            ctx, buf, &s->partial_specializations);
        break;
      case kStruct_deduction_guides:
        SReadRefVector(ctx, buf, kSerialKindSymbol, &s->deduction_guides);
        break;
      case kStruct_member_using_declarations:
        ReadMemberUsingVector(ctx, buf, &s->member_using_declarations);
        break;
      case kStruct_friend_type_declarations:
        ReadFriendTypeVector(ctx, buf, &s->friend_type_declarations);
        break;
      case kStruct_meta_aggregate_complete:
        WireReadBool(buf, &s->meta_aggregate_complete);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

static void WriteTokenSequenceToken(SerializeContext* ctx, WireBuffer* out,
                                    TokenSequenceToken* token) {
  if (token == NULL) {
    return;
  }
  WireWriteInt32(out, kTokenSeqWire_kind, (int32_t)token->kind);
  if (token->spelling.value != NULL) {
    SWriteStringVal(ctx, out, kTokenSeqWire_spelling, &token->spelling);
  }
  WireWriteUint64(out, kTokenSeqWire_location, (uint64_t)token->location);
  WireWriteInt32(out, kTokenSeqWire_piece_kind, (int32_t)token->piece_kind);
  SWriteRef(ctx, out, kTokenSeqWire_pseudo_value, kSerialKindAST,
            token->pseudo_value);
}

static TokenSequenceToken* ReadTokenSequenceToken(DeserializeContext* ctx,
                                                  WireBuffer* in) {
  TokenSequenceToken* token = TokenSequenceTokenNew(
      kToken_bad, NULL, 0, SOURCE_LOCATION_MISSING, NULL);
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kTokenSeqWire_kind: {
        int32_t kind = 0;
        WireReadInt32(in, &kind);
        token->kind = (Token)kind;
        break;
      }
      case kTokenSeqWire_spelling:
        SReadStringVal(ctx, in, &token->spelling);
        break;
      case kTokenSeqWire_location: {
        uint64_t location = 0;
        WireReadUint64(in, &location);
        token->location = (SourceLocation)location;
        break;
      }
      case kTokenSeqWire_piece_kind: {
        int32_t piece_kind = 0;
        WireReadInt32(in, &piece_kind);
        token->piece_kind = (TokenSequencePieceKind)piece_kind;
        break;
      }
      case kTokenSeqWire_pseudo_value:
        token->pseudo_value = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      default:
        WireSkip(in, wt);
        break;
    }
  }
  return token;
}

static void WriteTokenSequenceTokenVector(SerializeContext* ctx,
                                          WireBuffer* out, int field,
                                          Vector* tokens) {
  WireBuffer payload;
  WireBufferInitOwned(&payload, 16);
  size_t count = tokens != NULL ? tokens->length : 0;
  WireWriteRawVarint(&payload, count);
  for (size_t i = 0; i < count; i++) {
    WireBuffer element;
    WireBufferInitOwned(&element, 16);
    WriteTokenSequenceToken(ctx, &element,
                            (TokenSequenceToken*)tokens->value.p[i]);
    WireWriteRawVarint(&payload, (uint64_t)WireBufferSize(&element));
    WireWriteRaw(&payload, WireBufferData(&element), WireBufferSize(&element));
    WireBufferDestruct(&element);
  }
  WireWriteBytes(out, field, WireBufferData(&payload), WireBufferSize(&payload));
  WireBufferDestruct(&payload);
}

static void ReadTokenSequenceTokenVector(DeserializeContext* ctx, WireBuffer* in,
                                         Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count = 0;
  if (!WireReadRawVarint(&sub, &count)) {
    return;
  }
  for (uint64_t i = 0; i < count; i++) {
    const void* elem;
    size_t elen;
    if (!WireReadBytes(&sub, &elem, &elen)) {
      break;
    }
    WireBuffer er;
    WireBufferInitReader(&er, elem, elen);
    TokenSequenceToken* token = ReadTokenSequenceToken(ctx, &er);
    if (token != NULL) {
      VectorAppend(out, token);
    }
  }
}

static void WriteReflectionValueInline(SerializeContext* ctx, WireBuffer* out,
                                       ReflectionValue* reflection) {
  if (reflection == NULL) {
    return;
  }
  WireWriteInt32(out, kTArg_reflection_kind, (int32_t)reflection->kind);
  SWriteRef(ctx, out, kTArg_reflection_type, kSerialKindType,
            reflection->reflected_type);
  SWriteRef(ctx, out, kTArg_reflection_symbol, kSerialKindSymbol,
            reflection->symbol);
  SWriteRef(ctx, out, kTArg_reflection_member, kSerialKindStructMember,
            reflection->member);
  SWriteRef(ctx, out, kTArg_reflection_namespace, kSerialKindNamespace,
            reflection->namespace_);
  SWriteRef(ctx, out, kTArg_reflection_parent, kSerialKindStruct,
            reflection->parent_class);
  WireWriteUint64(out, kTArg_reflection_base_index,
                  (uint64_t)reflection->base_index);
  WireWriteUint64(out, kTArg_reflection_location,
                  (uint64_t)reflection->location);
  WireWriteUint64(out, kTArg_reflection_parameter_index,
                  (uint64_t)reflection->parameter_index);
  WireWriteInt64(out, kTArg_reflection_scalar_ivalue, reflection->scalar_ivalue);
  WireWriteDouble(out, kTArg_reflection_scalar_fvalue,
                  reflection->scalar_fvalue);
  WireWriteBool(out, kTArg_reflection_scalar_is_float,
                reflection->scalar_is_float);
  SWriteRef(ctx, out, kTArg_reflection_namespace_alias_target,
            kSerialKindNamespace, reflection->namespace_alias_target);
  SWriteRef(ctx, out, kTArg_reflection_constexpr_initializer, kSerialKindAST,
            reflection->constexpr_initializer);
  SWriteRef(ctx, out, kTArg_reflection_promoted_symbol, kSerialKindSymbol,
            reflection->promoted_symbol);
  SWriteRef(ctx, out, kTArg_reflection_substituted_template, kSerialKindSymbol,
            reflection->substituted_template);
  SWriteRef(ctx, out, kTArg_reflection_extract_type, kSerialKindType,
            reflection->extract_type);
  if (reflection->data_member_spec != NULL) {
    ReflectionDataMemberSpec* spec = reflection->data_member_spec;
    SWriteRef(ctx, out, kTArg_reflection_dms_member_type, kSerialKindType,
              spec->member_type);
    if (spec->name.value != NULL) {
      WireWriteString(out, kTArg_reflection_dms_name, spec->name.value,
                      spec->name.length);
    }
    WireWriteUint64(out, kTArg_reflection_dms_alignment,
                    (uint64_t)spec->alignment);
    WireWriteUint64(out, kTArg_reflection_dms_bit_width,
                    (uint64_t)spec->bit_width);
    WireWriteBool(out, kTArg_reflection_dms_no_unique_address,
                  spec->no_unique_address);
    WireWriteBool(out, kTArg_reflection_dms_has_name, spec->has_name);
    WireWriteBool(out, kTArg_reflection_dms_has_alignment, spec->has_alignment);
    WireWriteBool(out, kTArg_reflection_dms_has_bit_width, spec->has_bit_width);
  }
  if (reflection->enumerator_spec != NULL) {
    ReflectionEnumeratorSpec* spec = reflection->enumerator_spec;
    if (spec->name.value != NULL) {
      SWriteStringVal(ctx, out, kTArg_reflection_ens_name, &spec->name);
    }
    WireWriteInt64(out, kTArg_reflection_ens_value, spec->value);
    WireWriteBool(out, kTArg_reflection_ens_has_value, spec->has_value);
    WireWriteBool(out, kTArg_reflection_ens_has_name, spec->has_name);
  }
}

static ReflectionValue* ReadReflectionValueInline(DeserializeContext* ctx,
                                                  WireBuffer* in) {
  ReflectionValue* reflection = NULL;
  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    if (field == kTArg_reflection_kind) {
      int32_t kind = 0;
      WireReadInt32(in, &kind);
      reflection = ReflectionCreateDeserialized((ReflectionEntityKind)kind,
                                                SOURCE_LOCATION_MISSING);
      continue;
    }
    if (reflection == NULL) {
      reflection = ReflectionCreateDeserialized(kReflectionInvalid,
                                              SOURCE_LOCATION_MISSING);
    }
    switch (field) {
      case kTArg_reflection_type:
        reflection->reflected_type =
            (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kTArg_reflection_symbol:
        reflection->symbol = (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_reflection_member:
        reflection->member =
            (StructMember*)SReadRef(ctx, in, kSerialKindStructMember);
        break;
      case kTArg_reflection_namespace:
        reflection->namespace_ =
            (Namespace*)SReadRef(ctx, in, kSerialKindNamespace);
        break;
      case kTArg_reflection_parent:
        reflection->parent_class =
            (Struct*)SReadRef(ctx, in, kSerialKindStruct);
        break;
      case kTArg_reflection_base_index: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        reflection->base_index = (size_t)value;
        break;
      }
      case kTArg_reflection_location: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        reflection->location = (SourceLocation)value;
        break;
      }
      case kTArg_reflection_parameter_index: {
        uint64_t value = 0;
        WireReadUint64(in, &value);
        reflection->parameter_index = (size_t)value;
        break;
      }
      case kTArg_reflection_scalar_ivalue:
        WireReadInt64(in, &reflection->scalar_ivalue);
        break;
      case kTArg_reflection_scalar_fvalue:
        WireReadDouble(in, &reflection->scalar_fvalue);
        break;
      case kTArg_reflection_scalar_is_float:
        WireReadBool(in, &reflection->scalar_is_float);
        break;
      case kTArg_reflection_namespace_alias_target:
        reflection->namespace_alias_target =
            (Namespace*)SReadRef(ctx, in, kSerialKindNamespace);
        break;
      case kTArg_reflection_constexpr_initializer:
        reflection->constexpr_initializer =
            (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kTArg_reflection_promoted_symbol:
        reflection->promoted_symbol =
            (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_reflection_substituted_template:
        reflection->substituted_template =
            (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kTArg_reflection_extract_type:
        reflection->extract_type =
            (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        TypeRecordIncRef(reflection->extract_type);
        break;
      case kTArg_reflection_dms_member_type:
        if (reflection->data_member_spec == NULL) {
          reflection->data_member_spec = ReflectionDataMemberSpecNew(NULL);
        }
        reflection->data_member_spec->member_type =
            (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        TypeRecordIncRef(reflection->data_member_spec->member_type);
        break;
      case kTArg_reflection_dms_name:
        if (reflection->data_member_spec == NULL) {
          reflection->data_member_spec = ReflectionDataMemberSpecNew(NULL);
        }
        SReadStringVal(ctx, in, &reflection->data_member_spec->name);
        break;
      case kTArg_reflection_dms_alignment:
      case kTArg_reflection_dms_bit_width:
      case kTArg_reflection_dms_no_unique_address:
      case kTArg_reflection_dms_has_name:
      case kTArg_reflection_dms_has_alignment:
      case kTArg_reflection_dms_has_bit_width:
        if (reflection->data_member_spec == NULL) {
          reflection->data_member_spec = ReflectionDataMemberSpecNew(NULL);
        }
        if (field == kTArg_reflection_dms_alignment) {
          uint64_t alignment = 0;
          WireReadUint64(in, &alignment);
          reflection->data_member_spec->alignment = (size_t)alignment;
        } else if (field == kTArg_reflection_dms_bit_width) {
          uint64_t bit_width = 0;
          WireReadUint64(in, &bit_width);
          reflection->data_member_spec->bit_width = (size_t)bit_width;
        } else if (field == kTArg_reflection_dms_no_unique_address) {
          WireReadBool(in, &reflection->data_member_spec->no_unique_address);
        } else if (field == kTArg_reflection_dms_has_name) {
          WireReadBool(in, &reflection->data_member_spec->has_name);
        } else if (field == kTArg_reflection_dms_has_alignment) {
          WireReadBool(in, &reflection->data_member_spec->has_alignment);
        } else {
          WireReadBool(in, &reflection->data_member_spec->has_bit_width);
        }
        break;
      case kTArg_reflection_ens_name:
        if (reflection->enumerator_spec == NULL) {
          reflection->enumerator_spec = ReflectionEnumeratorSpecNew();
        }
        SReadStringVal(ctx, in, &reflection->enumerator_spec->name);
        break;
      case kTArg_reflection_ens_value:
        if (reflection->enumerator_spec == NULL) {
          reflection->enumerator_spec = ReflectionEnumeratorSpecNew();
        }
        WireReadInt64(in, &reflection->enumerator_spec->value);
        break;
      case kTArg_reflection_ens_has_value:
        if (reflection->enumerator_spec == NULL) {
          reflection->enumerator_spec = ReflectionEnumeratorSpecNew();
        }
        WireReadBool(in, &reflection->enumerator_spec->has_value);
        break;
      case kTArg_reflection_ens_has_name:
        if (reflection->enumerator_spec == NULL) {
          reflection->enumerator_spec = ReflectionEnumeratorSpecNew();
        }
        WireReadBool(in, &reflection->enumerator_spec->has_name);
        break;
      default:
        if (SerialReadReflectionExtendedField(ctx, in, field, reflection)) {
          break;
        }
        WireSkip(in, wt);
        break;
    }
  }
  return reflection;
}

static void WriteReflectionValueVector(SerializeContext* ctx, WireBuffer* out,
                                       int field, Vector* values) {
  WireBuffer payload;
  WireBufferInitOwned(&payload, 16);
  size_t count = values != NULL ? values->length : 0;
  WireWriteRawVarint(&payload, count);
  for (size_t i = 0; i < count; i++) {
    WireBuffer element;
    WireBufferInitOwned(&element, 16);
    WriteReflectionValueInline(ctx, &element,
                               (ReflectionValue*)values->value.p[i]);
    WireWriteRawVarint(&payload, (uint64_t)WireBufferSize(&element));
    WireWriteRaw(&payload, WireBufferData(&element), WireBufferSize(&element));
    WireBufferDestruct(&element);
  }
  WireWriteBytes(out, field, WireBufferData(&payload), WireBufferSize(&payload));
  WireBufferDestruct(&payload);
}

static void ReadReflectionValueVector(DeserializeContext* ctx, WireBuffer* in,
                                      Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count = 0;
  if (!WireReadRawVarint(&sub, &count)) {
    return;
  }
  for (uint64_t i = 0; i < count; i++) {
    const void* elem;
    size_t elen;
    if (!WireReadBytes(&sub, &elem, &elen)) {
      break;
    }
    WireBuffer er;
    WireBufferInitReader(&er, elem, elen);
    ReflectionValue* value = ReadReflectionValueInline(ctx, &er);
    if (value != NULL) {
      VectorAppend(out, ReflectionCanonicalize(value));
    }
  }
}

void SerialWriteReflectionExtendedPayload(SerializeContext* ctx, WireBuffer* buf,
                                          int field_sequence,
                                          int field_substituted_arguments,
                                          int field_dms_annotations,
                                          int field_token_sequence,
                                          ReflectionValue* value) {
  if (value == NULL) {
    return;
  }
  if (value->sequence.length > 0) {
    WriteReflectionValueVector(ctx, buf, field_sequence, &value->sequence);
  }
  if (value->substituted_arguments.length > 0) {
    SerialWriteTemplateArgumentVector(ctx, buf, field_substituted_arguments,
                                      &value->substituted_arguments);
  }
  if (value->data_member_spec != NULL &&
      value->data_member_spec->annotations.length > 0) {
    WriteReflectionValueVector(ctx, buf, field_dms_annotations,
                               &value->data_member_spec->annotations);
  }
  if (value->enumerator_spec != NULL &&
      value->enumerator_spec->attributes.length > 0) {
    WriteReflectionValueVector(ctx, buf, kTArg_reflection_ens_attributes,
                               &value->enumerator_spec->attributes);
  }
  if (value->enumerator_spec != NULL &&
      value->enumerator_spec->annotations.length > 0) {
    WriteReflectionValueVector(ctx, buf, kTArg_reflection_ens_annotations,
                               &value->enumerator_spec->annotations);
  }
  if (value->token_sequence.length > 0) {
    WriteTokenSequenceTokenVector(ctx, buf, field_token_sequence,
                                  &value->token_sequence);
  }
}

bool SerialReadReflectionExtendedField(DeserializeContext* ctx, WireBuffer* in,
                                       int field, ReflectionValue* value) {
  if (value == NULL) {
    return false;
  }
  if (field == kTArg_reflection_sequence) {
    ReadReflectionValueVector(ctx, in, &value->sequence);
    return true;
  }
  if (field == kTArg_reflection_substituted_arguments) {
    Vector* args = SerialReadTemplateArgumentVector(ctx, in);
    if (args != NULL) {
      for (size_t i = 0; i < args->length; i++) {
        VectorAppend(&value->substituted_arguments, args->value.p[i]);
      }
      VectorDelete(args);
    }
    return true;
  }
  if (field == kTArg_reflection_dms_annotations) {
    if (value->data_member_spec == NULL) {
      value->data_member_spec = ReflectionDataMemberSpecNew(NULL);
    }
    ReadReflectionValueVector(ctx, in, &value->data_member_spec->annotations);
    return true;
  }
  if (field == kTArg_reflection_ens_attributes) {
    if (value->enumerator_spec == NULL) {
      value->enumerator_spec = ReflectionEnumeratorSpecNew();
    }
    ReadReflectionValueVector(ctx, in, &value->enumerator_spec->attributes);
    return true;
  }
  if (field == kTArg_reflection_ens_annotations) {
    if (value->enumerator_spec == NULL) {
      value->enumerator_spec = ReflectionEnumeratorSpecNew();
    }
    ReadReflectionValueVector(ctx, in, &value->enumerator_spec->annotations);
    return true;
  }
  if (field == kTArg_reflection_token_sequence) {
    ReadTokenSequenceTokenVector(ctx, in, &value->token_sequence);
    return true;
  }
  return false;
}

void SerializeRegisterTypeKinds(void) {
  static const SerialKindVtable type_vt = {WriteType, AllocType, ReadType,
                                           "Type"};
  static const SerialKindVtable enum_vt = {WriteEnum, AllocEnum, ReadEnum,
                                           "Enum"};
  static const SerialKindVtable member_vt = {WriteMember, AllocMember,
                                             ReadMember, "StructMember"};
  static const SerialKindVtable struct_vt = {WriteStruct, AllocStruct,
                                             ReadStruct, "Struct"};
  SerializeRegisterKind(kSerialKindType, &type_vt);
  SerializeRegisterKind(kSerialKindEnum, &enum_vt);
  SerializeRegisterKind(kSerialKindStructMember, &member_vt);
  SerializeRegisterKind(kSerialKindStruct, &struct_vt);
  SerializeRegisterFields(kSerialKindType, kTypeFields,
                          sizeof(kTypeFields) / sizeof(kTypeFields[0]));
  SerializeRegisterFields(kSerialKindEnum, kEnumFields,
                          sizeof(kEnumFields) / sizeof(kEnumFields[0]));
  SerializeRegisterFields(kSerialKindStructMember, kMemberFields,
                          sizeof(kMemberFields) / sizeof(kMemberFields[0]));
  SerializeRegisterFields(kSerialKindStruct, kStructFields,
                          sizeof(kStructFields) / sizeof(kStructFields[0]));
}
