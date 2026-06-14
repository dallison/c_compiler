//
//  symbol.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "symbol.h"
#include "type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstring.h"
#include "compiler.h"

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
  sym->value.fvalue = 0;
  sym->stack_offset = 0;
  sym->alias_target = NULL;
  sym->location = 0;
  sym->usage_info.reads = 0;
  sym->usage_info.used_as_arg = 0;
  sym->usage_info.used_in_loop = 0;
  sym->id = compiler->next_symbol_id++;
  VectorInit(&sym->attributes);
  sym->alignment = 0;
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
}

void SymbolDelete(Symbol* symbol) {
  SymbolDestruct(symbol);
  free(symbol);
}

void SymbolAddAttribute(Symbol* symbol, Attribute* attribute) {
  VectorAppend(&symbol->attributes, attribute);   // Takes ownership.
}

Symbol* SymbolClone(Symbol* sym) {
  Symbol* new_sym = NewSymbol(sym->name.value, sym->type, sym->storage);
  new_sym->flags = sym->flags;
  new_sym->usage_info = sym->usage_info;
  new_sym->value = sym->value;
  new_sym->stack_offset = sym->stack_offset;
  new_sym->alias_target = sym->alias_target;
  new_sym->location = sym->location;
  new_sym->alignment = sym->alignment;
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
