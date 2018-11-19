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
}

void SymbolDelete(Symbol* symbol) {
  SymbolDestruct(symbol);
  free(symbol);
}

void SymbolPrintDetails(Symbol* sym, bool with_function_body) {
  const char* storage = "unknown";
  switch (sym->storage) {
    case kStorageImplicit:
      storage = "";
      break;
    case kStorageAuto:
      storage = "auto ";
      break;
    case kStorageStatic:
      storage = "static ";
      break;
    case kStorageTypedef:
      storage = "typedef ";
      break;
    case kStorageExtern:
      storage = "extern ";
      break;
    case kStorageRegister:
      storage = "register ";
      break;
    case kStorageAssembler:
      storage = "assembler";
      break;
  }
  printf("%s: %s", sym->name.value, storage);
  TypeRecordPrintDetails(sym->type, with_function_body);
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
