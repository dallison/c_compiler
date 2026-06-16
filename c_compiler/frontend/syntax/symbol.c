//
//  symbol.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "symbol.h"
#include "type.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstring.h"
#include "compiler.h"
#include "symbol_table.h"

bool StorageIs(Storage storage, Storage value) {
  return (storage & value) != 0;
}

// Normalizes an attribute name by stripping a surrounding "__" pair, so that
// "__packed__" is treated the same as "packed" (as GCC does).
static void NormalizeAttributeName(String* name) {
  size_t len = name->length;
  if (len >= 5 && name->value[0] == '_' && name->value[1] == '_' &&
      name->value[len - 1] == '_' && name->value[len - 2] == '_') {
    String stripped = {0};
    StringInitFromSegment(&stripped, name->value + 2, len - 4);
    StringSetString(name, &stripped);
    StringDestruct(&stripped);
  }
}

Attribute* NewAttribute(const char* name) {
  Attribute* attr = malloc(sizeof(Attribute));
  StringInit(&attr->name, name);
  NormalizeAttributeName(&attr->name);
  VectorInit(&attr->args);
  return attr;
}

void AttributeDestruct(Attribute* attr) {
  StringDestruct(&attr->name);
  VectorDestructWithContents(&attr->args, (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
}

void AttributeDelete(Attribute* attr) {
  AttributeDestruct(attr);
  free(attr);
}

void AttributeAddArg(Attribute* attr, const char* arg, size_t length) {
  VectorAppend(&attr->args, NewStringWithLength(arg, length));
}

Attribute* AttributeClone(Attribute* attr) {
  Attribute* copy = malloc(sizeof(Attribute));
  StringInit(&copy->name, attr->name.value);
  VectorInit(&copy->args);
  for (size_t i = 0; i < attr->args.length; i++) {
    String* a = attr->args.value.p[i];
    VectorAppend(&copy->args, NewString(a->value));
  }
  return copy;
}

size_t AttributeArgCount(Attribute* attr) { return attr->args.length; }

const char* AttributeArgString(Attribute* attr, size_t index) {
  if (index >= attr->args.length) {
    return NULL;
  }
  return ((String*)attr->args.value.p[index])->value;
}

bool AttributeArgInt(Attribute* attr, size_t index, long* value) {
  const char* s = AttributeArgString(attr, index);
  if (s == NULL || *s == '\0') {
    return false;
  }
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') {
    return false;
  }
  *value = v;
  return true;
}

Attribute* AttributeListFind(Vector* attrs, const char* name) {
  for (size_t i = 0; i < attrs->length; i++) {
    Attribute* attr = attrs->value.p[i];
    if (StringEqual(&attr->name, name)) {
      return attr;
    }
  }
  return NULL;
}

bool AttributeListHas(Vector* attrs, const char* name) {
  return AttributeListFind(attrs, name) != NULL;
}

void AttributeListDestruct(Vector* attrs) {
  VectorDestructWithContents(attrs, (VectorElementDestructor)AttributeDestruct,
                             /*free_element=*/true);
}

void AttributeListClone(Vector* dest, Vector* src) {
  VectorInit(dest);
  for (size_t i = 0; i < src->length; i++) {
    VectorAppend(dest, AttributeClone((Attribute*)src->value.p[i]));
  }
}

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage) {
  StringInit(&sym->name, name);
  StringInit(&sym->asm_name, NULL);
  sym->namespace_ = NULL;
  sym->type = NULL;
  sym->storage = storage;
  sym->flags.is_defined = false;
  sym->flags.is_tentative_decl = false;
  sym->flags.is_forward_declared = false;
  sym->flags.is_local = false;
  sym->flags.is_argument = false;
  sym->flags.is_temp = false;
  sym->flags.address_taken = false;
  sym->flags.used = false;
  sym->flags.invented = false;
  sym->flags.is_inline_defn = false;
  sym->flags.value_set = false;
  sym->flags.noreturn = false;
  sym->flags.always_inline = false;
  sym->flags.noinline = false;
  sym->flags.is_using_alias = false;
  sym->flags.is_overloaded = false;
  sym->flags.is_template = false;
  sym->flags.is_template_parameter = false;
  sym->flags.is_template_type_parameter = false;
  sym->value.fvalue = 0;
  sym->stack_offset = 0;
  sym->alias_target = NULL;
  sym->overload_next = NULL;
  sym->location = 0;
  sym->usage_info.reads = 0;
  sym->usage_info.used_as_arg = 0;
  sym->usage_info.used_in_loop = 0;
  sym->id = compiler->next_symbol_id++;
  VectorInit(&sym->attributes);
  sym->alignment = 0;
  sym->template_parameter_index = -1;
  SymbolSetType(sym, type);
  sym->die = NULL;
}

Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage) {
  Symbol* sym = malloc(sizeof(Symbol));
  SymbolInit(sym, name, type, storage);
  return sym;
}

void SymbolDestruct(Symbol* symbol) {
  StringDestruct(&symbol->name);
  StringDestruct(&symbol->asm_name);
  TypeRecordDelete(symbol->type);
  AttributeListDestruct(&symbol->attributes);
  if (symbol->overload_next != NULL) {
    SymbolDelete(symbol->overload_next);
  }
}

void SymbolDelete(Symbol* symbol) {
  SymbolDestruct(symbol);
  free(symbol);
}

void SymbolAddAttribute(Symbol* symbol, Attribute* attribute) {
  VectorAppend(&symbol->attributes, attribute);   // Takes ownership.
}

static bool CXXSymbolShouldMangle(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type)) {
    return false;
  }
  if (symbol->asm_name.length != 0) {
    return false;
  }
  if (strcmp(symbol->name.value, "main") == 0 &&
      symbol->type->info.function.cxx_member_owner == NULL &&
      symbol->namespace_ == NULL) {
    return false;
  }
  return true;
}

typedef struct {
  const char* name;
  const char* encoding;
} CXXOperatorEncodingEntry;

static int CompareCXXOperatorEncodingEntry(const void* key,
                                           const void* element) {
  const char* name = key;
  const CXXOperatorEncodingEntry* entry = element;
  return strcmp(name, entry->name);
}

static const char* CXXOperatorEncoding(const char* name) {
  static const CXXOperatorEncodingEntry entries[] = {
      {"operator delete", "dl"},
      {"operator delete[]", "da"},
      {"operator new", "nw"},
      {"operator new[]", "na"},
      {"operator!", "nt"},
      {"operator!=", "ne"},
      {"operator%", "rm"},
      {"operator%=", "rM"},
      {"operator&", "an"},
      {"operator&&", "aa"},
      {"operator&=", "aN"},
      {"operator()", "cl"},
      {"operator*", "ml"},
      {"operator*=", "mL"},
      {"operator+", "pl"},
      {"operator++", "pp"},
      {"operator+=", "pL"},
      {"operator,", "cm"},
      {"operator-", "mi"},
      {"operator--", "mm"},
      {"operator-=", "mI"},
      {"operator->", "pt"},
      {"operator->*", "pm"},
      {"operator/", "dv"},
      {"operator/=", "dV"},
      {"operator<", "lt"},
      {"operator<<", "ls"},
      {"operator<<=", "lS"},
      {"operator<=", "le"},
      {"operator=", "aS"},
      {"operator==", "eq"},
      {"operator>", "gt"},
      {"operator>=", "ge"},
      {"operator>>", "rs"},
      {"operator>>=", "rS"},
      {"operator[]", "ix"},
      {"operator^", "eo"},
      {"operator^=", "eO"},
      {"operator|", "or"},
      {"operator|=", "oR"},
      {"operator||", "oo"},
      {"operator~", "co"},
  };
  const CXXOperatorEncodingEntry* entry =
      bsearch(name, entries, sizeof(entries) / sizeof(entries[0]),
              sizeof(entries[0]), CompareCXXOperatorEncodingEntry);
  return entry != NULL ? entry->encoding : NULL;
}

static void AppendCXXNameComponent(String* out, const char* name) {
  const char* op_encoding = CXXOperatorEncoding(name);
  if (op_encoding != NULL) {
    StringAppend(out, op_encoding);
    return;
  }
  String sanitized;
  StringInit(&sanitized, NULL);
  bool needs_sanitizing = false;
  for (const char* p = name; *p != '\0'; p++) {
    char ch = *p;
    if (isalnum((unsigned char)ch) || ch == '_') {
      StringAppendChar(&sanitized, ch);
    } else {
      needs_sanitizing = true;
      StringAppendChar(&sanitized, '_');
    }
  }
  const char* component = needs_sanitizing ? sanitized.value : name;
  String length;
  StringInit(&length, NULL);
  StringPrintf(&length, "%zu", strlen(component));
  StringAppendString(out, &length);
  StringAppend(out, component);
  StringDestruct(&length);
  StringDestruct(&sanitized);
}

static void AppendCXXNestedNamespaceComponents(String* out, Namespace* ns) {
  if (ns == NULL || ns->parent == NULL) {
    return;
  }
  AppendCXXNestedNamespaceComponents(out, ns->parent);
  if (ns->name.length != 0) {
    AppendCXXNameComponent(out, ns->name.value);
  }
}

static void AppendCXXUnqualifiedName(String* out, Symbol* symbol) {
  TypeRecord* func = symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  if (func->info.function.is_constructor) {
    if (owner == NULL || owner->tag_name == NULL) {
      AppendCXXNameComponent(out, symbol->name.value);
    }
    StringAppend(out, "C1");
    return;
  }
  if (func->info.function.is_destructor) {
    if (owner == NULL || owner->tag_name == NULL) {
      AppendCXXNameComponent(out, symbol->name.value);
    }
    StringAppend(out, "D1");
    return;
  }
  AppendCXXNameComponent(out, symbol->name.value);
}

static bool CXXNameNeedsNestedEncoding(Symbol* symbol) {
  return symbol->namespace_ != NULL ||
         symbol->type->info.function.cxx_member_owner != NULL;
}

static void AppendCXXName(String* out, Symbol* symbol) {
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  if (!CXXNameNeedsNestedEncoding(symbol)) {
    AppendCXXUnqualifiedName(out, symbol);
    return;
  }

  StringAppendChar(out, 'N');
  if (symbol->type->info.function.is_const_member) {
    StringAppendChar(out, 'K');
  }
  AppendCXXNestedNamespaceComponents(out, symbol->namespace_);
  if (owner != NULL && owner->tag_name != NULL) {
    AppendCXXNameComponent(out, owner->tag_name->value);
  }
  AppendCXXUnqualifiedName(out, symbol);
  StringAppendChar(out, 'E');
}

static void AppendCXXTypeEncoding(String* out, TypeRecord* type) {
  if (type == NULL) {
    StringAppendChar(out, 'v');
    return;
  }
  if (TypeIsConst(type)) {
    StringAppendChar(out, 'K');
  }
  if (TypeIsVolatile(type)) {
    StringAppendChar(out, 'V');
  }
  switch (type->declarator) {
    case kDeclPointer:
      StringAppendChar(out, 'P');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclReference:
      StringAppendChar(out, 'R');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclRValueReference:
      StringAppendChar(out, 'O');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclArray:
      StringAppendChar(out, 'P');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclFunction:
      StringAppendChar(out, 'F');
      AppendCXXTypeEncoding(out, type->next);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        AppendCXXTypeEncoding(out, formal->type);
      }
      StringAppendChar(out, 'E');
      return;
    case kDeclPrimitive:
      break;
  }

  if (TypeIsVoid(type)) {
    StringAppendChar(out, 'v');
  } else if (TypeIsBool(type)) {
    StringAppendChar(out, 'b');
  } else if (TypeIsChar(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'h' : 'c');
  } else if (TypeIsShort(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 't' : 's');
  } else if (TypeIsLongLong(type)) {
    StringAppend(out, TypeIsUnsigned(type) ? "y" : "x");
  } else if (TypeIsLong(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'm' : 'l');
  } else if (TypeIsInt(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'j' : 'i');
  } else if (TypeIsFloat(type)) {
    StringAppendChar(out, 'f');
  } else if (TypeIsDouble(type)) {
    StringAppendChar(out, 'd');
  } else if (TypeIsLongDouble(type)) {
    StringAppendChar(out, 'e');
  } else if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
             type->info.struct_info->tag_name != NULL) {
    AppendCXXNameComponent(out, type->info.struct_info->tag_name->value);
  } else if (TypeIsEnum(type) && type->info.enum_info != NULL &&
             type->info.enum_info->tag_name != NULL) {
    AppendCXXNameComponent(out, type->info.enum_info->tag_name->value);
  } else {
    StringAppendChar(out, 'v');
  }
}

static void AppendCXXFunctionParameterTypes(String* out, Symbol* symbol) {
  TypeRecord* func = symbol->type;
  bool has_implicit_this =
      func->info.function.prototype.length > 0 &&
      strcmp(((Symbol*)func->info.function.prototype.value.p[0])->name.value,
             "this") == 0;
  size_t first_arg = has_implicit_this ? 1 : 0;
  if (func->info.function.prototype.length <= first_arg) {
    StringAppendChar(out, 'v');
    return;
  }
  for (size_t i = first_arg; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    AppendCXXTypeEncoding(out, formal->type);
  }
}

void SymbolSetCXXMangledAsmName(Symbol* symbol) {
  if (!CXXSymbolShouldMangle(symbol)) {
    return;
  }
  String mangled;
  StringInit(&mangled, NULL);
  if (compiler->prepend_underscore) {
    StringAppendChar(&mangled, '_');
  }
  StringAppend(&mangled, "_Z");
  AppendCXXName(&mangled, symbol);
  AppendCXXFunctionParameterTypes(&mangled, symbol);
  StringSetString(&symbol->asm_name, &mangled);
  StringDestruct(&mangled);
}

Symbol* SymbolClone(Symbol* sym) {
  Symbol* new_sym = NewSymbol(sym->name.value, sym->type, sym->storage);
  new_sym->flags = sym->flags;
  new_sym->usage_info = sym->usage_info;
  new_sym->value = sym->value;
  new_sym->stack_offset = sym->stack_offset;
  new_sym->alias_target = sym->alias_target;
  new_sym->overload_next = NULL;
  new_sym->location = sym->location;
  new_sym->alignment = sym->alignment;
  new_sym->template_parameter_index = sym->template_parameter_index;
  new_sym->namespace_ = sym->namespace_;
  StringSetString(&new_sym->asm_name, &sym->asm_name);
  // NewSymbol already initialized new_sym->attributes; replace it with a deep
  // copy of the source's attributes.
  VectorDestruct(&new_sym->attributes);
  AttributeListClone(&new_sym->attributes, &sym->attributes);
  return new_sym;
}

bool SymbolHasAttribute(Symbol* symbol, const char* attribute) {
  return AttributeListHas(&symbol->attributes, attribute);
}

Attribute* SymbolFindAttribute(Symbol* symbol, const char* attribute) {
  return AttributeListFind(&symbol->attributes, attribute);
}

static const char* storages[] = {
  "",
  "auto ",
  "static ",
  "typedef ",
  "extern ",
  "register ",
  "assembler ",
  "__thread ",
};

void SymbolPrintDetails(Symbol* sym, bool with_function_body, FILE* fp) {
  String storage;
  StringInit(&storage, "");
  for (int i = 0; i < 32; i++) {
    if (StorageIs(sym->storage, 1<<i)) {
      StringAppend(&storage, storages[i]);
    }
  }
 
  fprintf(fp, "%s: %s", sym->name.value, storage.value);
  TypeRecordPrintDetails(sym->type, with_function_body, fp);
  StringDestruct(&storage);
}

void SymbolPrint(Symbol* sym, FILE* fp) { SymbolPrintDetails(sym, false, fp); }

void SymbolSetType(Symbol* symbol, struct TypeRecord* type) {
  if (symbol->type == type) {
    // Already set
    return;
  }
  if (symbol->type != NULL) {
    TypeRecordDelete(symbol->type);
  }
  symbol->type = type;
  TypeRecordIncRef(type);
}
