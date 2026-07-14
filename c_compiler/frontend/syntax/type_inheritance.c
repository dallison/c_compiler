//
//  type_inheritance.c
//  c_compiler
//
#include "type_class_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static bool CXXMemberFunctionSignaturesMatch(TypeRecord* a, TypeRecord* b);

bool StructHasBaseStruct(Struct* str, Struct* target, int* offset) {
  if (str == NULL || target == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == target) {
      if (offset != NULL) {
        *offset = base->byte_offset;
      }
      return true;
    }
    int nested_offset = 0;
    if (StructHasBaseStruct(base_struct, target, &nested_offset)) {
      if (offset != NULL) {
        *offset = base->byte_offset + nested_offset;
      }
      return true;
    }
  }
  return false;
}

bool StructHasBaseType(Syntax* syntax, Struct* str, TypeRecord* target,
                              int* offset) {
  if (str == NULL || target == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    TypeRecord* base_type =
        TypeMaterializeClassTemplateSpecialization(syntax, base->type);
    bool matches = TypeEqual(base_type, target);
    if (matches) {
      if (offset != NULL) {
        *offset = base->byte_offset;
      }
      if (base_type != base->type) {
        TypeRecordDelete(base_type);
      }
      return true;
    }
    if (TypeIsStructOrUnion(base_type) && base_type->info.struct_info != NULL) {
      int nested_offset = 0;
      if (StructHasBaseType(syntax, base_type->info.struct_info, target,
                            &nested_offset)) {
        if (offset != NULL) {
          *offset = base->byte_offset + nested_offset;
        }
        if (base_type != base->type) {
          TypeRecordDelete(base_type);
        }
        return true;
      }
    }
    if (base_type != base->type) {
      TypeRecordDelete(base_type);
    }
  }
  return false;
}

static CXXAccess ParseBaseAccess(TypeParser* parser, bool is_class) {
  if (LexMatch(parser->lex, TOK(public))) {
    return kAccessPublic;
  }
  if (LexMatch(parser->lex, TOK(protected))) {
    return kAccessProtected;
  }
  if (LexMatch(parser->lex, TOK(private))) {
    return kAccessPrivate;
  }
  return is_class ? kAccessPrivate : kAccessPublic;
}

bool ParseCXXClassFinalSpecifier(TypeParser* parser) {
  if (!CompilerIsCXX() || !LexLookingAt(parser->lex, TOK(identifier)) ||
      !StringEqual(&parser->lex->spelling, "final")) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  if (LexLookingAt(parser->lex, TOK(lbrace)) ||
      LexLookingAt(parser->lex, TOK(colon))) {
    LexCheckpointDestruct(&checkpoint);
    return true;
  }
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return false;
}

void ParseCXXBaseSpecifiers(TypeParser* parser, Vector* bases,
                                   bool is_union, bool is_class) {
  if (!CompilerIsCXX() || is_union || !LexMatch(parser->lex, TOK(colon))) {
    return;
  }
  do {
    bool is_virtual = false;
    if (LexMatch(parser->lex, TOK(virtual))) {
      is_virtual = true;
    }
    CXXAccess access = ParseBaseAccess(parser, is_class);
    if (LexMatch(parser->lex, TOK(virtual))) {
      is_virtual = true;
    }
    TypeRecord* base_type = TypeParserParseType(parser, true);
    bool is_pack_expansion = CompilerIsCXX() &&
                             LexMatch(parser->lex, TOK(ellipsis));
    int placeholder_index = -1;
    bool is_template_parameter_base =
        TypeIsTemplateParameterPlaceholder(base_type, &placeholder_index);
    if (is_pack_expansion &&
        !CurrentTemplateParameterIsPack(parser->syntax, placeholder_index)) {
      SyntaxError(parser->syntax,
                  "base class pack expansion requires a template parameter pack");
      TypeRecordDelete(base_type);
      continue;
    }
    if (base_type == NULL ||
        (!is_template_parameter_base &&
         (!TypeIsStructOrUnion(base_type) ||
          base_type->info.struct_info == NULL))) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    if (!is_template_parameter_base && base_type->info.struct_info->is_final) {
      const char* base_name =
          base_type->info.struct_info->tag_name != NULL
              ? base_type->info.struct_info->tag_name->value
              : "<anonymous>";
      SyntaxError(parser->syntax, "cannot derive from final base class %s",
                  base_name);
    }
    if (!is_template_parameter_base) {
      TypeRecordCalculateSize(base_type);
    }
    CXXBaseSpecifier* base =
        NewCXXBaseSpecifier(base_type, access, is_virtual);
    base->is_pack_expansion = is_pack_expansion;
    VectorAppend(bases, base);
    // `base_type` was returned with refs==0 (a fresh, unowned copy) and
    // NewCXXBaseSpecifier took the single owning reference for the base
    // specifier.  Do NOT release it here: doing so would drop the count back to
    // zero and free the record while the base specifier still points at it.
    // That is harmless for bases whose distinguishing state lives on a
    // registered struct, but for a deferred dependent template-id base (e.g.
    // `integral_constant<size_t, N>` parsed inside a class template) the
    // template arguments live only on this record, so freeing it silently
    // dropped them and broke later instantiation of the derived template.
  } while (LexMatch(parser->lex, TOK(comma)));
}

void LayoutCXXBaseSpecifiers(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->is_virtual) {
      continue;
    }
    if (base->type == NULL || !TypeIsStructOrUnion(base->type)) {
      continue;
    }
    AlignNextOffset(str, base->type);
    base->byte_offset = str->next_offset;
    int base_size = base->type->size;
    if (TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->non_virtual_size > 0) {
      base_size = base->type->info.struct_info->non_virtual_size;
    }
    if (!str->is_union) {
      str->next_offset += base_size;
      str->size = str->next_offset;
    } else if (base->type->size > str->size) {
      str->size = base->type->size;
    }
  }
}

static bool SameStructType(TypeRecord* left, TypeRecord* right) {
  return left != NULL && right != NULL && TypeIsStructOrUnion(left) &&
         TypeIsStructOrUnion(right) && left->info.struct_info != NULL &&
         left->info.struct_info == right->info.struct_info;
}

static CXXVirtualBaseInfo* FindCXXVirtualBaseInfo(Struct* str,
                                                  TypeRecord* type) {
  if (str == NULL || type == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (SameStructType(base->type, type)) {
      return base;
    }
  }
  return NULL;
}

static CXXVirtualBaseInfo* AddCXXVirtualBaseInfo(Struct* str,
                                                 TypeRecord* type,
                                                 CXXAccess access) {
  CXXVirtualBaseInfo* existing = FindCXXVirtualBaseInfo(str, type);
  if (existing != NULL) {
    if (access == kAccessPrivate || existing->access == kAccessPrivate) {
      existing->access = kAccessPrivate;
    } else if (access == kAccessProtected ||
               existing->access == kAccessProtected) {
      existing->access = kAccessProtected;
    }
    return existing;
  }
  CXXVirtualBaseInfo* base =
      NewCXXVirtualBaseInfo(type, access, (int)str->virtual_bases.length);
  VectorAppend(&str->virtual_bases, base);
  return base;
}

static void CollectCXXVirtualBasesFromBase(Struct* str,
                                           CXXBaseSpecifier* base) {
  if (str == NULL || base == NULL || base->type == NULL ||
      !TypeIsStructOrUnion(base->type) ||
      base->type->info.struct_info == NULL) {
    return;
  }
  Struct* base_struct = base->type->info.struct_info;
  if (base->is_virtual) {
    AddCXXVirtualBaseInfo(str, base->type, base->access);
  }
  for (size_t i = 0; i < base_struct->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* inherited = base_struct->virtual_bases.value.p[i];
    AddCXXVirtualBaseInfo(str, inherited->type, inherited->access);
  }
}

void CollectCXXVirtualBases(Struct* str) {
  if (!CompilerIsCXX() || str == NULL) {
    return;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CollectCXXVirtualBasesFromBase(str, str->bases.value.p[i]);
  }
}

bool StructHasVirtualBases(Struct* str) {
  return str != NULL && str->virtual_bases.length > 0;
}

void CopyCXXBaseVirtualMembers(Struct* str) {
  if (!CompilerIsCXX() || str->virtual_members.length != 0) {
    return;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    for (size_t j = 0; j < base_struct->virtual_members.length; j++) {
      VectorAppend(&str->virtual_members, base_struct->virtual_members.value.p[j]);
    }
  }
}

static bool StructHasPolymorphicBase(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->virtual_members.length > 0) {
      return true;
    }
  }
  return false;
}

static TypeRecord* NewCXXVTableEntryType(void) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  return NewPointerTo(kQualPlain, void_type);
}

static TypeRecord* NewCXXVPtrType(void) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
  return NewPointerTo(kQualPlain, entry_type);
}

static TypeRecord* NewCXXVBPtrType(void) {
  TypeRecord* entry_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  return NewPointerTo(kQualPlain, entry_type);
}

void AddCXXVPtrMember(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str->vptr_member != NULL ||
      str->virtual_members.length == 0 || StructHasPolymorphicBase(str)) {
    return;
  }
  int ptr_size = SizeofPointer();
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    base->byte_offset += ptr_size;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member->is_static || member->is_member_function ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    member->byte_offset += ptr_size;
  }
  str->next_offset += ptr_size;
  str->current_offset += ptr_size;
  str->size += ptr_size;
  if (ptr_size > str->alignment) {
    str->alignment = ptr_size;
  }

  Symbol* symbol =
      NewSymbol("__vptr", NewCXXVPtrType(), STO(implicit));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  StructMember* member = NewStructMember(symbol);
  member->access = kAccessPublic;
  member->byte_offset = 0;
  str->vptr_member = member;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

void AddCXXVBPtrMember(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->vbptr_member != NULL ||
      str->virtual_bases.length == 0) {
    return;
  }
  Symbol* symbol =
      NewSymbol("__vbptr", NewCXXVBPtrType(), STO(implicit));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  StructMember* member = NewStructMember(symbol);
  member->access = kAccessPublic;
  AlignNextOffset(str, symbol->type);
  member->byte_offset = str->next_offset;
  member->index = str->members.length;
  UpdateStructSize(str, symbol->type, str->is_union);
  str->vbptr_member = member;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

void LayoutCXXVirtualBaseSpecifiers(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->virtual_bases.length == 0) {
    return;
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    AlignNextOffset(str, base->type);
    base->byte_offset = str->next_offset;
    if (!str->is_union) {
      str->next_offset += base->type->size;
      str->size = str->next_offset;
    } else if (base->type->size > str->size) {
      str->size = base->type->size;
    }
  }
}

static TypeRecord* NewCXXVTableType(size_t slots) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)slots, false);
  TypeRecordChain(array_type, entry_type);
  TypeRecordCalculateSize(array_type);
  return array_type;
}

static bool CXXVirtualNamesCompatible(StructMember* candidate,
                                      StructMember* base_member) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL || base_member == NULL ||
      base_member->symbol == NULL || base_member->symbol->type == NULL) {
    return false;
  }
  TypeRecord* candidate_func = candidate->symbol->type;
  TypeRecord* base_func = base_member->symbol->type;
  if (candidate_func->info.function.is_destructor &&
      base_func->info.function.is_destructor) {
    return true;
  }
  return StringEqualString(&candidate->symbol->name, &base_member->symbol->name);
}

static StructMember* FindCXXOverriderInHierarchy(Struct* str,
                                                 StructMember* base_member) {
  if (str == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* candidate = str->members.value.p[i];
    if (candidate == NULL || !candidate->is_member_function ||
        candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !candidate->symbol->type->info.function.is_virtual ||
        !CXXVirtualNamesCompatible(candidate, base_member)) {
      continue;
    }
    if (CXXMemberFunctionSignaturesMatch(candidate->symbol->type,
                                         base_member->symbol->type)) {
      return candidate;
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    StructMember* found =
        FindCXXOverriderInHierarchy(base->type->info.struct_info, base_member);
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

static StructMember* FindCXXFinalOverrider(Struct* complete,
                                           StructMember* base_member) {
  if (complete == NULL || base_member == NULL || base_member->symbol == NULL ||
      base_member->symbol->type == NULL) {
    return base_member;
  }
  // Walk the whole complete-object hierarchy, not just its directly declared
  // members: an intermediate base (e.g. `B` in `A <- B <- C`) may carry the
  // final overrider that a grandbase-source subobject vtable must point at.
  StructMember* overrider = FindCXXOverriderInHierarchy(complete, base_member);
  return overrider != NULL ? overrider : base_member;
}

static Symbol* RegisterCXXThisAdjustorThunk(TypeParser* parser, Symbol* target,
                                            int adjustment) {
  if (adjustment == 0 || target == NULL) {
    return target;
  }
  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk = compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk->target == target && thunk->this_adjustment == adjustment) {
      return thunk->thunk;
    }
  }
  String name;
  StringInit(&name, "__davecc_this_adjustor_");
  char suffix[64];
  snprintf(suffix, sizeof(suffix), "%zu_%d_",
           compiler->cxx_this_adjustor_thunks.length,
           adjustment < 0 ? -adjustment : adjustment);
  StringAppend(&name, suffix);
  StringAppendString(&name, &target->name);
  for (size_t i = 0; i < name.length; i++) {
    char ch = name.value[i];
    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9') || ch == '_')) {
      name.value[i] = '_';
    }
  }
  Symbol* thunk_symbol =
      NewSymbol(name.value, TypeRecordCopy(target->type), STO(static));
  thunk_symbol->flags.invented = true;
  thunk_symbol->flags.is_defined = true;
  thunk_symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, thunk_symbol);
  StringDestruct(&name);

  CXXThisAdjustorThunk* thunk = malloc(sizeof(CXXThisAdjustorThunk));
  thunk->thunk = thunk_symbol;
  thunk->target = target;
  thunk->this_adjustment = adjustment;
  VectorAppend(&compiler->cxx_this_adjustor_thunks, thunk);
  return thunk_symbol;
}

static TypeRecord* NewCXXVBTableType(size_t slots) {
  TypeRecord* entry_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)slots, false);
  TypeRecordChain(array_type, entry_type);
  TypeRecordCalculateSize(array_type);
  return array_type;
}

void UpdateCXXAbstractStatus(Struct* str) {
  str->is_abstract = false;
  if (!CompilerIsCXX()) {
    return;
  }
  for (size_t i = 0; i < str->virtual_members.length; i++) {
    StructMember* member = str->virtual_members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        TypeIsFunction(member->symbol->type) &&
        member->symbol->type->info.function.is_pure_virtual) {
      str->is_abstract = true;
      return;
    }
  }
}

static Symbol* RegisterCXXVTableForSubobject(TypeParser* parser,
                                             Struct* complete,
                                             Struct* source,
                                             int source_offset) {
  if (!CompilerIsCXX() || complete == NULL || source == NULL ||
      source->virtual_members.length == 0 || complete->tag_name == NULL ||
      source->tag_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vtable_symbols.length; i++) {
    CXXVTableInfo* info = complete->vtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }

  String name;
  StringInit(&name, "__davecc_vtbl_");
  StringAppendString(&name, complete->tag_name);
  if (!(complete == source && source_offset == 0)) {
    StringAppend(&name, "_");
    StringAppendString(&name, source->tag_name);
    StringAppend(&name, "_");
    char offset_suffix[32];
    snprintf(offset_suffix, sizeof(offset_suffix), "%d", source_offset);
    StringAppend(&name, offset_suffix);
  }
  // The vtable carries two RTTI header entries (offset_to_top and a pointer to
  // the complete object's type_info) immediately before the function pointers.
  // __vptr still points at the first function pointer (entry index 2) so that
  // virtual_index-based dispatch is unchanged.
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVTableType(source->virtual_members.length + 2),
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, symbol);
  if (complete == source && source_offset == 0) {
    complete->vtable_symbol = symbol;
  }
  StringDestruct(&name);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = false;
  var->is_weak = false;
  var->size = symbol->type->size;
  var->alignment = TypeRecordAlignment(symbol->type->next);
  VectorInit(&var->initializers);
  var->is_tls = false;
  var->is_local = false;

  int ptr_size = SizeofPointer();
  // Header entry 0: offset_to_top -- the byte distance from this subobject's
  // vptr back to the most-derived object (0 for the primary table).
  Initializer* ott = malloc(sizeof(Initializer));
  ott->offset = 0;
  switch (ptr_size) {
    case 8:
      ott->type = kInitTypeLong;
      ott->value._long = (uint64_t)(int64_t)(-source_offset);
      break;
    case 2:
      ott->type = kInitTypeHalf;
      ott->value.half = (uint16_t)(-source_offset);
      break;
    default:
      ott->type = kInitTypeWord;
      ott->value.word = (uint32_t)(-source_offset);
      break;
  }
  VectorAppend(&var->initializers, ott);
  // Header entry 1: pointer to the complete object's type_info.
  TypeRecord* complete_type = NewTypeRecord(kTypeStruct, kQualPlain);
  complete_type->info.struct_info = complete;
  TypeRecordCalculateSize(complete_type);
  Symbol* type_info = RttiGetTypeInfoSymbol(complete_type);
  Initializer* ti = malloc(sizeof(Initializer));
  ti->offset = (int32_t)ptr_size;
  if (type_info != NULL) {
    ti->type = kInitTypeSymbol;
    ti->value.symbol = type_info;
  } else {
    ti->type = (ptr_size == 8) ? kInitTypeLong : kInitTypeWord;
    ti->value._long = 0;
  }
  VectorAppend(&var->initializers, ti);

  for (size_t i = 0; i < source->virtual_members.length; i++) {
    StructMember* member = source->virtual_members.value.p[i];
    member = FindCXXFinalOverrider(complete, member);
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    Initializer* init = malloc(sizeof(Initializer));
    init->offset = (int32_t)((i + 2) * SizeofPointer());
    if (member->symbol->type->info.function.is_pure_virtual) {
      if (SizeofPointer() == 8) {
        init->type = kInitTypeLong;
        init->value._long = 0;
      } else {
        init->type = kInitTypeWord;
        init->value.word = 0;
      }
    } else {
      init->type = kInitTypeSymbol;
      int adjustment = member->symbol->type->info.function.cxx_member_owner ==
                               complete
                           ? -source_offset
                           : 0;
      init->value.symbol =
          RegisterCXXThisAdjustorThunk(parser, member->symbol, adjustment);
    }
    VectorAppend(&var->initializers, init);
  }
  VectorAppend(&compiler->initialized_static_variables, var);
  CXXVTableInfo* info = malloc(sizeof(CXXVTableInfo));
  info->source = source;
  info->source_offset = source_offset;
  info->symbol = symbol;
  VectorAppend(&complete->vtable_symbols, info);
  return symbol;
}

static void RegisterCXXVSubobjectTables(TypeParser* parser,
                                        Struct* complete,
                                        Struct* source,
                                        int source_offset) {
  if (source == NULL) {
    return;
  }
  if (source->virtual_members.length > 0) {
    RegisterCXXVTableForSubobject(parser, complete, source, source_offset);
  }
  for (size_t i = 0; i < source->bases.length; i++) {
    CXXBaseSpecifier* base = source->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    RegisterCXXVSubobjectTables(parser, complete, base->type->info.struct_info,
                                source_offset + base->byte_offset);
  }
}

void RegisterCXXVTable(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->vtable_symbol != NULL ||
      str->virtual_members.length == 0 || str->tag_name == NULL) {
    return;
  }
  RegisterCXXVSubobjectTables(parser, str, str, 0);
}

static Symbol* RegisterCXXVBTableForSubobject(TypeParser* parser,
                                              Struct* complete,
                                              Struct* source,
                                              int source_offset) {
  if (!CompilerIsCXX() || complete == NULL || source == NULL ||
      source->virtual_bases.length == 0 || complete->tag_name == NULL ||
      source->tag_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = complete->vbtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  String name;
  StringInit(&name, "__davecc_vbtbl_");
  StringAppendString(&name, complete->tag_name);
  StringAppend(&name, "_");
  StringAppendString(&name, source->tag_name);
  StringAppend(&name, "_");
  char offset_suffix[32];
  snprintf(offset_suffix, sizeof(offset_suffix), "%d", source_offset);
  StringAppend(&name, offset_suffix);
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVBTableType(source->virtual_bases.length),
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, symbol);
  if (complete == source && source_offset == 0) {
    complete->vbtable_symbol = symbol;
  }
  StringDestruct(&name);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = false;
  var->is_weak = false;
  var->size = symbol->type->size;
  var->alignment = TypeRecordAlignment(symbol->type->next);
  VectorInit(&var->initializers);
  var->is_tls = false;
  var->is_local = false;
  for (size_t i = 0; i < source->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* source_base = source->virtual_bases.value.p[i];
    CXXVirtualBaseInfo* complete_base =
        FindCXXVirtualBaseInfo(complete, source_base->type);
    int offset = complete_base != NULL
                     ? complete_base->byte_offset - source_offset
                     : source_base->byte_offset;
    Initializer* init = malloc(sizeof(Initializer));
    init->offset = (int32_t)(i * (size_t)SizeofType(kTypeInt));
    init->type = kInitTypeWord;
    init->value.word = (uint32_t)offset;
    VectorAppend(&var->initializers, init);
  }
  VectorAppend(&compiler->initialized_static_variables, var);

  CXXVBTableInfo* info = malloc(sizeof(CXXVBTableInfo));
  info->source = source;
  info->source_offset = source_offset;
  info->symbol = symbol;
  VectorAppend(&complete->vbtable_symbols, info);
  return symbol;
}

static void RegisterCXXVBSubobjectTables(TypeParser* parser,
                                         Struct* complete,
                                         Struct* source,
                                         int source_offset) {
  if (source == NULL) {
    return;
  }
  if (source->virtual_bases.length > 0) {
    RegisterCXXVBTableForSubobject(parser, complete, source, source_offset);
  }
  for (size_t i = 0; i < source->bases.length; i++) {
    CXXBaseSpecifier* base = source->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    RegisterCXXVBSubobjectTables(parser, complete,
                                 base->type->info.struct_info,
                                 source_offset + base->byte_offset);
  }
}

void RegisterCXXVBTables(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->virtual_bases.length == 0 ||
      str->tag_name == NULL) {
    return;
  }
  RegisterCXXVBSubobjectTables(parser, str, str, 0);
}

Symbol* StructFindVBTableSymbol(Struct* complete, Struct* source,
                                int source_offset) {
  if (complete == NULL || source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = complete->vbtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  return NULL;
}

Symbol* StructFindVTableSymbol(Struct* complete, Struct* source,
                               int source_offset) {
  if (complete == NULL || source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vtable_symbols.length; i++) {
    CXXVTableInfo* info = complete->vtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  return NULL;
}

static bool CXXMemberFunctionSignaturesMatch(TypeRecord* a, TypeRecord* b) {
  if (!TypeIsFunction(a) || !TypeIsFunction(b) ||
      !TypeEqual(a->next, b->next) ||
      a->info.function.is_const_member != b->info.function.is_const_member ||
      a->info.function.is_volatile_member !=
          b->info.function.is_volatile_member ||
      a->info.function.ref_qualifier != b->info.function.ref_qualifier) {
    return false;
  }
  size_t a_first = a->info.function.cxx_member_owner != NULL ? 1 : 0;
  size_t b_first = b->info.function.cxx_member_owner != NULL ? 1 : 0;
  if (a->info.function.prototype.length - a_first !=
      b->info.function.prototype.length - b_first) {
    return false;
  }
  for (size_t i = 0; i < a->info.function.prototype.length - a_first; i++) {
    Symbol* a_arg = a->info.function.prototype.value.p[i + a_first];
    Symbol* b_arg = b->info.function.prototype.value.p[i + b_first];
    if (!TypeEqual(a_arg->type, b_arg->type)) {
      return false;
    }
  }
  return true;
}

static StructMember* FindCXXBaseVirtualOverride(Struct* str,
                                                StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL ||
      !member->is_member_function) {
    return NULL;
  }
  TypeRecord* func = member->symbol->type;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    String destructor_name = {0};
    StructMember* base_member = NULL;
    if (func->info.function.is_destructor) {
      StringInit(&destructor_name, "~");
      StringAppendString(&destructor_name,
                         base->type->info.struct_info->tag_name);
      base_member = FindStructMember(base->type->info.struct_info,
                                     &destructor_name);
    } else {
      base_member =
          FindStructMember(base->type->info.struct_info, &member->symbol->name);
    }
    for (StructMember* candidate = base_member; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->is_member_function &&
          candidate->symbol->type->info.function.is_virtual &&
          CXXMemberFunctionSignaturesMatch(member->symbol->type,
                                           candidate->symbol->type)) {
        if (destructor_name.value != NULL) {
          StringDestruct(&destructor_name);
        }
        return candidate;
      }
    }
    if (destructor_name.value != NULL) {
      StringDestruct(&destructor_name);
    }
  }
  return NULL;
}

int CXXBaseOffsetForMember(Struct* str, StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      member->symbol->type->info.function.cxx_member_owner == NULL) {
    return 0;
  }
  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  if (owner == str) {
    return 0;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == owner) {
      return base->byte_offset;
    }
    int nested_offset = CXXBaseOffsetForMember(base_struct, member);
    if (nested_offset != 0 || base_struct == owner) {
      return base->byte_offset + nested_offset;
    }
  }
  return 0;
}

void RegisterCXXVirtualMember(TypeParser* parser, Struct* str,
                                     StructMember* member) {
  if (!CompilerIsCXX() || str == NULL || member == NULL ||
      !member->is_member_function || member->is_static ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return;
  }
  TypeRecord* func = member->symbol->type;
  StructMember* override = FindCXXBaseVirtualOverride(str, member);
  if (func->info.function.is_override && override == NULL) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s marked override but does not override%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (override != NULL) {
    if (override->symbol->type->info.function.is_final) {
      String suffix;
      StringInit(&suffix, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
      SyntaxError(parser->syntax, "%s overrides final function%s",
                  member->symbol->name.value, suffix.value);
      StringDestruct(&suffix);
    }
    func->info.function.is_virtual = true;
    func->info.function.virtual_index =
        override->symbol->type->info.function.virtual_index;
    member->cxx_vcall_offset = CXXBaseOffsetForMember(str, override);
  }
  if (func->info.function.is_final && !func->info.function.is_virtual) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s marked final but is not virtual%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (func->info.function.is_pure_virtual && !func->info.function.is_virtual) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s is pure but is not virtual%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (!func->info.function.is_virtual) {
    return;
  }
  if (func->info.function.virtual_index < 0) {
    func->info.function.virtual_index = (int)str->virtual_members.length;
    VectorAppend(&str->virtual_members, member);
    return;
  }
  if (member->cxx_vcall_offset != 0) {
    return;
  }
  size_t index = (size_t)func->info.function.virtual_index;
  while (str->virtual_members.length <= index) {
    VectorAppend(&str->virtual_members, NULL);
  }
  VectorSet(&str->virtual_members, index, member);
}
