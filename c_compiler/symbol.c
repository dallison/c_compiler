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
#include "dstring.h"
#include "compiler.h"

bool StorageIs(Storage storage, Storage value) {
  return (storage & value) != 0;
}

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage) {
  StringInit(&sym->name, name);
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
  sym->value.fvalue = 0;
  sym->stack_offset = 0;
  sym->location = 0;
  sym->usage_info.reads = 0;
  sym->usage_info.used_as_arg = 0;
  sym->usage_info.used_in_loop = 0;
  sym->id = compiler->next_symbol_id++;
  VectorInit(&sym->attributes);
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
  TypeRecordDelete(symbol->type);
  VectorDestructWithContents(&symbol->attributes, (VectorElementDestructor)StringDestruct);
}

void SymbolDelete(Symbol* symbol) {
  SymbolDestruct(symbol);
  free(symbol);
}

void SymbolAddAttribute(Symbol* symbol, String* attribute) {
  VectorAppend(&symbol->attributes, attribute);   // Takes ownership.
}

Symbol* SymbolClone(Symbol* sym) {
  Symbol* new_sym = NewSymbol(sym->name.value, sym->type, sym->storage);
  new_sym->flags = sym->flags;
  new_sym->usage_info = sym->usage_info;
  new_sym->value = sym->value;
  new_sym->stack_offset = sym->stack_offset;
  new_sym->location = sym->location;
  VectorInit(&new_sym->attributes);
  for (size_t i = 0; i < sym->attributes.length; i++) {
    String* attr = sym->attributes.value.p[i];
    VectorAppend(&new_sym->attributes, NewString(attr->value));
  }
  return new_sym;
}

bool SymbolHasAttribute(Symbol* symbol, const char* attribute) {
  for (size_t i = 0; i < symbol->attributes.length; i++) {
    if (StringEqual(symbol->attributes.value.p[i], attribute)) {
      return true;
    }
  }
  return false;
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
