//
//  rtti.c
//  c_compiler
//
//  Emission of C++ std::type_info objects for RTTI (typeid / dynamic_cast) and
//  for the per-class vtable RTTI slot.  The emitted layout must match the
//  <typeinfo> header and the dynamic_cast runtime (libc/cxx_rtti.c):
//
//    struct type_info  { const char* __name; const __base_info* __bases;
//                        long __base_count; };
//    struct __base_info { const type_info* __type; long __offset; };
//
//  All objects are emitted as weak globals so that identical types defined in
//  multiple translation units coalesce, giving typeid pointer-identity.
//

#include "rtti.h"

#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "dstring.h"
#include "symbol.h"
#include "syntax.h"
#include "type.h"

// Builds the canonical, sanitized mangled key for `type` into `out` (which the
// caller initializes and destructs).  Top-level cv-qualifiers are ignored so
// e.g. `const Foo` and `Foo` share one type_info.
static void RttiMangledKey(TypeRecord* type, String* out) {
  Qualifiers saved = type->qualifiers;
  type->qualifiers &= ~(kQualConst | kQualVolatile);
  AppendCXXMangledTypeName(out, type);
  type->qualifiers = saved;
  for (size_t i = 0; i < out->length; i++) {
    char ch = out->value[i];
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_';
    if (!valid) {
      out->value[i] = '_';
    }
  }
}

// Initializer width for a pointer-sized integer (offset_to_top, base offsets
// and counts all live in pointer-sized slots in this ABI).
static Initializer* NewIntPtrInitializer(int32_t offset, int64_t value) {
  Initializer* init = malloc(sizeof(Initializer));
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

static Initializer* NewSymbolInitializer(int32_t offset, Symbol* symbol) {
  Initializer* init = malloc(sizeof(Initializer));
  init->offset = offset;
  init->type = kInitTypeSymbol;
  init->value.symbol = symbol;
  return init;
}

// Creates a weak-global static symbol of opaque `byte_count` bytes whose
// initializers are supplied by the caller, registers it for emission and
// returns it.  `initializers` ownership transfers to the new variable.
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
  return symbol;
}

static Symbol* EmitTypeNameString(const char* key) {
  String name;
  StringInit(&name, "__davecc_tin_");
  StringAppend(&name, key);

  size_t len = strlen(key) + 1;  // include NUL terminator.
  Vector inits;
  VectorInit(&inits);
  Initializer* init = malloc(sizeof(Initializer));
  init->type = kInitTypeMemory;
  init->offset = 0;
  BufferInit(&init->value.memory);
  BufferAppend(&init->value.memory, (char*)key, len);
  VectorAppend(&inits, init);

  Symbol* symbol = EmitWeakStatic(name.value, len, &inits);
  StringDestruct(&name);
  return symbol;
}

Symbol* RttiGetTypeInfoSymbol(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  String key;
  StringInit(&key, "");
  RttiMangledKey(type, &key);
  Symbol* existing =
      MapFindPointerKey(&compiler->rtti_typeinfo_map, &key);
  if (existing != NULL) {
    StringDestruct(&key);
    return existing;
  }

  int ptr_size = SizeofPointer();

  // Reserve the type_info symbol up-front and record it before recursing into
  // bases so that cyclic / diamond base graphs terminate.
  String ti_name;
  StringInit(&ti_name, "__davecc_ti_");
  StringAppend(&ti_name, key.value);

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

  // The map owns an independent copy of the key String.
  String* map_key = NewString(key.value);
  MapInsert(&compiler->rtti_typeinfo_map,
            (MapKeyValue){.key.p = map_key, .value.p = ti_symbol});

  Symbol* name_symbol = EmitTypeNameString(key.value);

  // Direct base classes (non-virtual only; virtual bases are deferred).
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
      VectorAppend(&base_inits, NewSymbolInitializer(entry, base_ti));
      VectorAppend(&base_inits,
                   NewIntPtrInitializer(entry + ptr_size, base->byte_offset));
      base_count++;
    }
    if (base_count > 0) {
      String bi_name;
      StringInit(&bi_name, "__davecc_tib_");
      StringAppend(&bi_name, key.value);
      base_info_symbol = EmitWeakStatic(
          bi_name.value, (size_t)(base_count * 2 * ptr_size), &base_inits);
      StringDestruct(&bi_name);
    }
    VectorDestructWithContents(&base_inits, NULL, /*free_element=*/true);
  }

  // Emit the type_info object itself.
  Vector ti_inits;
  VectorInit(&ti_inits);
  VectorAppend(&ti_inits, NewSymbolInitializer(0, name_symbol));
  if (base_info_symbol != NULL) {
    VectorAppend(&ti_inits, NewSymbolInitializer(ptr_size, base_info_symbol));
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

  StringDestruct(&key);
  return ti_symbol;
}
