//
//  linker_dynamic.c
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_dynamic.h"
#include <stdlib.h>
#include "linker_file.h"
#include "linker_symbols.h"
#include "linker_reloc.h"
#include "linker.h"

void DynamicSectionInit(DynamicSection* s) {
  VectorInit(&s->global_offset_table);
  VectorInit(&s->procedure_linkage_table);
  VectorInit(&s->relocations);
}

DynamicSection* NewDynamicSection(void) {
  DynamicSection* s = malloc(sizeof(DynamicSection));
  DynamicSectionInit(s);
  return s;
}

void DynamicSectionDestruct(DynamicSection* s) {
  // TODO: vector contents.
  VectorDestruct(&s->global_offset_table);
  VectorDestruct(&s->procedure_linkage_table);
  VectorDestruct(&s->relocations);
}

void DynamicSectionDelete(DynamicSection* s) {
  DynamicSectionDestruct(s);
  free(s);
}

void DynamicSectionInventSymbols(struct Linker* linker, DynamicSection* s) {
  s->global_offset_table_symbol = LinkerInventSymbol(linker, "_GLOBAL_OFFSET_TABLE_", 8);
  s->dynamic_symbol = LinkerInventSymbol(linker, "_DYNAMIC_", 8);
}

DynamicLibrary* NewDynamicLibrary(LinkerFile* file) {
  DynamicLibrary* lib = malloc(sizeof(DynamicLibrary));
  lib->file = file;
  return lib;
}

void DynamicLibraryDestruct(DynamicLibrary* lib) {
  LinkerFileDelete(lib->file);
}

void DynamicLibraryDelete(DynamicLibrary* lib) {
  DynamicLibraryDestruct(lib);
  free(lib);
}

int GetGOTOffset(DynamicSection* s, LinkerSymbol* symbol, int reloc_type) {
  if (symbol->got_offset == -1) {
    // Symbol is not in Global Offset Table, add it.
    VectorAppend(&s->global_offset_table, symbol);
    symbol->got_offset = (int)s->global_offset_table.length - 1;
    
    // Add a relocation for it.
    LinkerRelocation* reloc = NewLinkerRelocation(symbol->name.value, symbol->got_offset * 8, reloc_type);
    VectorAppend(&s->relocations, reloc);
  }
  return symbol->got_offset;
}

int GetPLTOffset(DynamicSection* s, LinkerSymbol* symbol, int reloc_type) {
  GetGOTOffset(s, symbol, reloc_type);
  
  if (symbol->plt_offset == -1) {
    VectorAppend(&s->procedure_linkage_table, symbol);
    symbol->plt_offset = (int)s->procedure_linkage_table.length - 1;
  }
  return symbol->plt_offset;
}

#if 0
Section* DynamicInventSection(const char* name) {
  return NULL;
}
#endif

