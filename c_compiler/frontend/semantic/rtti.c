//
//  rtti.c
//  c_compiler
//
//  Emission of C++ std::type_info objects for RTTI (typeid / dynamic_cast) and
//  for the per-class vtable RTTI slot.
//
//  Itanium ABI targets emit weak _ZTS/_ZTI symbols with polymorphic type_info
//  layout (__class_type_info / __si_class_type_info / __vmi_class_type_info).
//  Legacy DaveCC RTTI (flat type_info + __davecc_ti_* names) is retained for
//  6502/65C02 and p-code where vtable/type_info overhead is gated off.
//

#include "rtti.h"

#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "dstring.h"
#include "symbol.h"
#include "syntax.h"
#include "type.h"

static const char kItaniumVptrClass[] =
    "_ZTVN10__cxxabiv117__class_type_infoE+16";
static const char kItaniumVptrSiClass[] =
    "_ZTVN10__cxxabiv120__si_class_type_infoE+16";
static const char kItaniumVptrVmiClass[] =
    "_ZTVN10__cxxabiv121__vmi_class_type_infoE+16";
static const char kItaniumVptrClass32[] =
    "_ZTVN10__cxxabiv117__class_type_infoE_u2b16";
static const char kItaniumVptrSiClass32[] =
    "_ZTVN10__cxxabiv120__si_class_type_infoE_u2b16";
static const char kItaniumVptrVmiClass32[] =
    "_ZTVN10__cxxabiv121__vmi_class_type_infoE_u2b16";

RttiABI RttiTargetABI(void) {
  if (compiler == NULL || compiler->target_name == NULL) {
    return kRttiABIItanium;
  }
  if (StringEqual(compiler->target_name, "6502") ||
      StringEqual(compiler->target_name, "65c02") ||
      StringEqual(compiler->target_name, "p-code") ||
      StringEqual(compiler->target_name, "pcode")) {
    return kRttiABIDaveCC;
  }
  return kRttiABIItanium;
}

bool RttiUsesItaniumABI(void) {
  return RttiTargetABI() == kRttiABIItanium;
}

static void RttiItaniumMangledKey(TypeRecord* type, String* out) {
  Qualifiers saved = type->qualifiers;
  type->qualifiers &= ~(kQualConst | kQualVolatile);
  AppendCXXMangledTypeName(out, type);
  type->qualifiers = saved;
}

static void RttiLegacyMangledKey(TypeRecord* type, String* out) {
  RttiItaniumMangledKey(type, out);
  for (size_t i = 0; i < out->length; i++) {
    char ch = out->value[i];
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_';
    if (!valid) {
      out->value[i] = '_';
    }
  }
}

static Initializer* NewIntPtrInitializer(int32_t offset, int64_t value) {
  Initializer* init = calloc(1, sizeof(Initializer));
  init->offset = offset;
  switch (SizeofPointer()) {
    case 8:
      init->type = kInitTypeLong;
      init->value._long = (uint64_t)value;
      break;
    case 2:
      init->type = kInitTypeHalf;
      init->value.half = (uint16_t)value;
      break;
    default:
      init->type = kInitTypeWord;
      init->value.word = (uint32_t)value;
      break;
  }
  return init;
}

static Initializer* NewWordInitializer(int32_t offset, uint32_t value) {
  Initializer* init = calloc(1, sizeof(Initializer));
  init->offset = offset;
  init->type = kInitTypeWord;
  init->value.word = value;
  return init;
}

static void SetItaniumAsmName(Symbol* symbol, const char* prefix,
                              const String* mangled) {
  String asm_name;
  StringInit(&asm_name, prefix);
  StringAppend(&asm_name, mangled->value);
  StringSetString(&symbol->asm_name, &asm_name);
  StringDestruct(&asm_name);
  free(symbol->cached_target_symbol_name);
  symbol->cached_target_symbol_name = NULL;
}

static Symbol* EmitWeakStatic(const char* name, size_t byte_count,
                              Vector* initializers) {
  TypeRecord* char_type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  TypeRecord* storage = NewBasicArrayTypeRecord(kQualPlain, (int)byte_count,
                                                false);
  TypeRecordChain(storage, char_type);
  TypeRecordCalculateSize(storage);

  Symbol* symbol = NewSymbol(name, storage, STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->flags.is_weak = true;
  SyntaxAddSymbol(&compiler->syntax, symbol);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = true;
  var->is_weak = true;
  var->size = storage->size;
  var->alignment = SizeofPointer();
  var->is_tls = false;
  var->is_local = false;
  var->initializers = *initializers;
  VectorInit(initializers);
  VectorAppend(&compiler->initialized_static_variables, var);
  CompilerRegisterLazyCXXStatic(var);
  return symbol;
}

static Symbol* GetItaniumVptrApSymbol(const char* name) {
  String lookup;
  StringInit(&lookup, name);
  Symbol* symbol = FindGlobalSymbol(&lookup);
  StringDestruct(&lookup);
  if (symbol != NULL) {
    return symbol;
  }

  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  symbol = NewSymbol(name, void_ptr, STO(extern));
  StringSet(&symbol->asm_name, name);
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

typedef enum {
  kItaniumRttiClass,
  kItaniumRttiSiClass,
  kItaniumRttiVmiClass,
} ItaniumRttiKind;

typedef struct {
  Symbol* base_ti;
  int byte_offset;
  bool is_public;
} ItaniumDirectBase;

static ItaniumRttiKind ClassifyItaniumClassRtti(Vector* direct_bases) {
  if (direct_bases->length == 0) {
    return kItaniumRttiClass;
  }
  if (direct_bases->length == 1) {
    ItaniumDirectBase* base = direct_bases->value.p[0];
    if (base->is_public && base->byte_offset == 0) {
      return kItaniumRttiSiClass;
    }
  }
  return kItaniumRttiVmiClass;
}

static Symbol* EmitItaniumTypeNameString(const String* mangled) {
  String name;
  StringInit(&name, "__davecc_zts_");
  StringAppend(&name, mangled->value);

  size_t len = mangled->length + 1;
  Vector inits;
  VectorInit(&inits);
  Initializer* init = calloc(1, sizeof(Initializer));
  init->type = kInitTypeMemory;
  init->offset = 0;
  BufferInit(&init->value.memory);
  BufferAppend(&init->value.memory, mangled->value, len);
  VectorAppend(&inits, init);

  Symbol* symbol = EmitWeakStatic(name.value, len, &inits);
  SetItaniumAsmName(symbol, "_ZTS", mangled);
  StringDestruct(&name);
  return symbol;
}

static Symbol* RttiGetTypeInfoSymbolItanium(TypeRecord* type, String* key) {
  int ptr_size = SizeofPointer();

  String ti_name;
  StringInit(&ti_name, "__davecc_zti_");
  StringAppend(&ti_name, key->value);

  Vector direct_bases;
  VectorInit(&direct_bases);
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base->type == NULL || base->is_virtual) {
        continue;
      }
      Symbol* base_ti = RttiGetTypeInfoSymbol(base->type);
      if (base_ti == NULL) {
        continue;
      }
      ItaniumDirectBase* entry = malloc(sizeof(ItaniumDirectBase));
      entry->base_ti = base_ti;
      entry->byte_offset = base->byte_offset;
      entry->is_public = base->access == kAccessPublic;
      VectorAppend(&direct_bases, entry);
    }
  }

  ItaniumRttiKind kind = kItaniumRttiClass;
  if (TypeIsStructOrUnion(type)) {
    kind = ClassifyItaniumClassRtti(&direct_bases);
  }

  size_t object_size = (size_t)(2 * ptr_size);
  const char* vptr_ap_name =
      ptr_size == 4 ? kItaniumVptrClass32 : kItaniumVptrClass;
  switch (kind) {
    case kItaniumRttiClass:
      object_size = (size_t)(2 * ptr_size);
      vptr_ap_name =
          ptr_size == 4 ? kItaniumVptrClass32 : kItaniumVptrClass;
      break;
    case kItaniumRttiSiClass:
      object_size = (size_t)(3 * ptr_size);
      vptr_ap_name =
          ptr_size == 4 ? kItaniumVptrSiClass32 : kItaniumVptrSiClass;
      break;
    case kItaniumRttiVmiClass:
      object_size =
          (size_t)(2 * ptr_size + 8 + direct_bases.length * 2 * ptr_size);
      vptr_ap_name =
          ptr_size == 4 ? kItaniumVptrVmiClass32 : kItaniumVptrVmiClass;
      break;
  }

  TypeRecord* char_type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  TypeRecord* ti_storage =
      NewBasicArrayTypeRecord(kQualPlain, (int)object_size, false);
  TypeRecordChain(ti_storage, char_type);
  TypeRecordCalculateSize(ti_storage);

  Symbol* ti_symbol = NewSymbol(ti_name.value, ti_storage, STO(static));
  ti_symbol->flags.invented = true;
  ti_symbol->flags.is_defined = true;
  ti_symbol->flags.is_weak = true;
  SyntaxAddSymbol(&compiler->syntax, ti_symbol);
  SetItaniumAsmName(ti_symbol, "_ZTI", key);
  StringDestruct(&ti_name);

  String* map_key = NewString(key->value);
  MapInsert(&compiler->rtti_typeinfo_map,
            (MapKeyValue){.key.p = map_key, .value.p = ti_symbol});

  Symbol* name_symbol = EmitItaniumTypeNameString(key);
  Symbol* vptr_ap = GetItaniumVptrApSymbol(vptr_ap_name);

  Vector ti_inits;
  VectorInit(&ti_inits);
  VectorAppend(&ti_inits, NewSymbolInitializer(0, vptr_ap, 0));
  VectorAppend(&ti_inits, NewSymbolInitializer(ptr_size, name_symbol, 0));

  switch (kind) {
    case kItaniumRttiClass:
      break;
    case kItaniumRttiSiClass: {
      ItaniumDirectBase* base = direct_bases.value.p[0];
      VectorAppend(&ti_inits,
                   NewSymbolInitializer(2 * ptr_size, base->base_ti, 0));
      break;
    }
    case kItaniumRttiVmiClass:
      VectorAppend(&ti_inits, NewWordInitializer(2 * ptr_size, 0));
      VectorAppend(&ti_inits,
                   NewWordInitializer(2 * ptr_size + 4,
                                      (uint32_t)direct_bases.length));
      for (size_t i = 0; i < direct_bases.length; i++) {
        ItaniumDirectBase* base = direct_bases.value.p[i];
        int32_t entry = (int32_t)(2 * ptr_size + 8 + i * 2 * ptr_size);
        VectorAppend(&ti_inits, NewSymbolInitializer(entry, base->base_ti, 0));
        long offset_flags = ((long)base->byte_offset << 8);
        if (base->is_public) {
          offset_flags |= 2;
        }
        VectorAppend(&ti_inits,
                     NewIntPtrInitializer(entry + ptr_size, offset_flags));
      }
      break;
  }

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = ti_symbol;
  var->is_global = true;
  var->is_weak = true;
  var->size = ti_storage->size;
  var->alignment = ptr_size;
  var->is_tls = false;
  var->is_local = false;
  var->initializers = ti_inits;
  VectorAppend(&compiler->initialized_static_variables, var);
  CompilerRegisterLazyCXXStatic(var);

  VectorDestructWithContents(&direct_bases, NULL, /*free_element=*/true);
  return ti_symbol;
}

static Symbol* EmitLegacyTypeNameString(const char* key) {
  String name;
  StringInit(&name, "__davecc_tin_");
  StringAppend(&name, key);

  size_t len = strlen(key) + 1;
  Vector inits;
  VectorInit(&inits);
  Initializer* init = calloc(1, sizeof(Initializer));
  init->type = kInitTypeMemory;
  init->offset = 0;
  BufferInit(&init->value.memory);
  BufferAppend(&init->value.memory, (char*)key, len);
  VectorAppend(&inits, init);

  Symbol* symbol = EmitWeakStatic(name.value, len, &inits);
  StringDestruct(&name);
  return symbol;
}

static Symbol* RttiGetTypeInfoSymbolLegacy(TypeRecord* type, String* key) {
  int ptr_size = SizeofPointer();

  String ti_name;
  StringInit(&ti_name, "__davecc_ti_");
  StringAppend(&ti_name, key->value);

  TypeRecord* char_type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  TypeRecord* ti_storage =
      NewBasicArrayTypeRecord(kQualPlain, 3 * ptr_size, false);
  TypeRecordChain(ti_storage, char_type);
  TypeRecordCalculateSize(ti_storage);
  Symbol* ti_symbol = NewSymbol(ti_name.value, ti_storage, STO(static));
  ti_symbol->flags.invented = true;
  ti_symbol->flags.is_defined = true;
  ti_symbol->flags.is_weak = true;
  SyntaxAddSymbol(&compiler->syntax, ti_symbol);
  StringDestruct(&ti_name);

  String* map_key = NewString(key->value);
  MapInsert(&compiler->rtti_typeinfo_map,
            (MapKeyValue){.key.p = map_key, .value.p = ti_symbol});

  Symbol* name_symbol = EmitLegacyTypeNameString(key->value);

  Symbol* base_info_symbol = NULL;
  int64_t base_count = 0;
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    Vector base_inits;
    VectorInit(&base_inits);
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base->type == NULL || base->is_virtual) {
        continue;
      }
      Symbol* base_ti = RttiGetTypeInfoSymbol(base->type);
      if (base_ti == NULL) {
        continue;
      }
      int32_t entry = (int32_t)(base_count * 2 * ptr_size);
      VectorAppend(&base_inits, NewSymbolInitializer(entry, base_ti, 0));
      VectorAppend(&base_inits,
                   NewIntPtrInitializer(entry + ptr_size, base->byte_offset));
      base_count++;
    }
    if (base_count > 0) {
      String bi_name;
      StringInit(&bi_name, "__davecc_tib_");
      StringAppend(&bi_name, key->value);
      base_info_symbol = EmitWeakStatic(
          bi_name.value, (size_t)(base_count * 2 * ptr_size), &base_inits);
      StringDestruct(&bi_name);
    }
    VectorDestructWithContents(&base_inits, NULL, /*free_element=*/true);
  }

  Vector ti_inits;
  VectorInit(&ti_inits);
  VectorAppend(&ti_inits, NewSymbolInitializer(0, name_symbol, 0));
  if (base_info_symbol != NULL) {
    VectorAppend(&ti_inits, NewSymbolInitializer(ptr_size, base_info_symbol, 0));
  } else {
    VectorAppend(&ti_inits, NewIntPtrInitializer(ptr_size, 0));
  }
  VectorAppend(&ti_inits, NewIntPtrInitializer(2 * ptr_size, base_count));

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = ti_symbol;
  var->is_global = true;
  var->is_weak = true;
  var->size = ti_storage->size;
  var->alignment = ptr_size;
  var->is_tls = false;
  var->is_local = false;
  var->initializers = ti_inits;
  VectorAppend(&compiler->initialized_static_variables, var);
  CompilerRegisterLazyCXXStatic(var);
  return ti_symbol;
}

Symbol* RttiGetTypeInfoSymbol(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }

  String key;
  StringInit(&key, "");
  if (RttiUsesItaniumABI()) {
    RttiItaniumMangledKey(type, &key);
  } else {
    RttiLegacyMangledKey(type, &key);
  }

  Symbol* existing = MapFindPointerKey(&compiler->rtti_typeinfo_map, &key);
  if (existing != NULL) {
    StringDestruct(&key);
    return existing;
  }

  Symbol* ti_symbol = RttiUsesItaniumABI()
                          ? RttiGetTypeInfoSymbolItanium(type, &key)
                          : RttiGetTypeInfoSymbolLegacy(type, &key);
  StringDestruct(&key);
  return ti_symbol;
}
