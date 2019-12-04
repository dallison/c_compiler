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

bool StorageIs(Storage storage, Storage value) {
  return (storage & value) != 0;
}

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage) {
  StringInit(&sym->name, name);
  sym->type = NULL;
  sym->storage = storage;
  sym->is_defined = false;
  sym->is_forward_declared = false;
  sym->is_local = false;
  sym->is_argument = false;
  sym->is_temp = false;
  sym->address_taken = false;
  sym->used = false;
  sym->value.fvalue = 0;
  sym->stack_offset = 0;
  VectorInit(&sym->attributes);
  SymbolSetType(sym, type);
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

void SymbolPrintDetails(Symbol* sym, bool with_function_body) {
  String storage;
  StringInit(&storage, "");
  for (int i = 0; i < 32; i++) {
    if (StorageIs(sym->storage, 1<<i)) {
      StringAppend(&storage, storages[i]);
    }
  }
 
  printf("%s: %s", sym->name.value, storage.value);
  TypeRecordPrintDetails(sym->type, with_function_body);
  StringDestruct(&storage);
}

void SymbolPrint(Symbol* sym) { SymbolPrintDetails(sym, false); }

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
