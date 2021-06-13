//
//  linker_file.c
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_file.h"
#include "linker_symbols.h"
#include "linker_reloc.h"
#include "linker.h"
#include <stdlib.h>

// Create a new linker file from an ELF file.
ObjectFile* NewObjectFile(ELFReaderFile* elf_file, Linker* linker, const char* filename) {
  ObjectFile* file = malloc(sizeof(ObjectFile));
  StringInit(&file->filename, filename);
  file->elf_file = elf_file;
  HashTableInit(&file->local_symbol_table, "local-symbol-table", 111,
                LinkerSymbolHash, LinkerSymbolInsertInHashTable, LinkerSymbolFindInHashTable);

  VectorInit(&file->relocations);
  file->linker = linker;
  MapInitForStringKeys(&file->sections_by_name);
  MapInitForInt64Keys(&file->sections_by_type);
  VectorInit(&file->common_symbols);
  return file;
}

static void DeleteSectionMapEntry(MapKeyValue* kv, void* data) {
  Vector* sections = kv->value.p;
  VectorDelete(sections);
}

void ObjectFileDestruct(ObjectFile* file) {
  StringDestruct(&file->filename);
  
  LinkerClearSymbolTable(&file->local_symbol_table);
  HashTableDestruct(&file->local_symbol_table);
  MapDestruct(&file->sections_by_name);

  VectorDestructWithContents(&file->relocations, (VectorElementDestructor)RelocationDestruct);

  MapTraverse(&file->sections_by_type, DeleteSectionMapEntry, NULL);
  MapDestruct(&file->sections_by_type);

  VectorDestruct(&file->common_symbols);
}

void ObjectFileDelete(ObjectFile* file) {
  ObjectFileDestruct(file);
  free(file);
}

// Find a symbol by looking in the given file and then in the global
// symbol table.
LinkerSymbol* ObjectFileFindSymbol(ObjectFile* file, const char *name) {
  LinkerSymbol* sym = LinkerFindSymbol(&file->local_symbol_table, name);
  if (sym == NULL) {
    sym = LinkerFindSymbol(&file->linker->global_symbol_table, name);
  }
  return sym;
}

// Find a section by name.
ELFReaderSection* ObjectFileFindSection(ObjectFile* file, String* name) {
  return MapFindPointerKey(&file->sections_by_name, name);
}


