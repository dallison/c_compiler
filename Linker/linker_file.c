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

// Map compare function for section names.  The key is a String*.
static int CompareMappedSectionNames(const void*a, const void* b) {
  const MapKeyValue* s1 = a;
  const MapKeyValue* s2 = b;
  return StringCompareString(s1->key, s2->key);
}

// Map compare function for section types.  The key is an integer.
static int CompareMappedSectionTypes(const void*a, const void* b) {
  const MapKeyValue* s1 = a;
  const MapKeyValue* s2 = b;
  return (int)(s1->key - s2->key);
}

// Create a new linker file from an ELF file.
LinkerFile* NewLinkerFile(ELFReaderFile* elf_file, Linker* linker, const char* filename) {
  LinkerFile* file = malloc(sizeof(LinkerFile));
  StringInit(&file->filename, filename);
  file->elf_file = elf_file;
  HashTableInit(&file->local_symbol_table, "local-symbol-table", 111,
                LinkerSymbolHash, LinkerSymbolInsertInHashTable, LinkerSymbolFindInHashTable);

  VectorInit(&file->relocations);
  file->linker = linker;
  MapInit(&file->sections_by_name, CompareMappedSectionNames);
  MapInit(&file->sections_by_type, CompareMappedSectionTypes);
  VectorInit(&file->common_symbols);
  return file;
}

static void DeleteSectionMapEntry(const void* key, void* value, void* data) {
  Vector* sections = value;
  VectorDelete(sections);
}

void LinkerFileDestruct(LinkerFile* file) {
  StringDestruct(&file->filename);
  
  LinkerClearSymbolTable(&file->local_symbol_table);
  HashTableDestruct(&file->local_symbol_table);
  MapDestruct(&file->sections_by_name);

  VectorDestructWithContents(&file->relocations, (VectorElementDestructor)LinkerRelocationDestruct);

  MapTraverse(&file->sections_by_type, DeleteSectionMapEntry, NULL);
  MapDestruct(&file->sections_by_type);

  VectorDestruct(&file->common_symbols);
}

void LinkerFileDelete(LinkerFile* file) {
  LinkerFileDestruct(file);
  free(file);
}

// Find a symbol by looking in the given file and then in the global
// symbol table.
LinkerSymbol* LinkerFileFindSymbol(LinkerFile* file, const char *name) {
  LinkerSymbol* sym = LinkerFindSymbol(&file->local_symbol_table, name);
  if (sym == NULL) {
    sym = LinkerFindSymbol(&file->linker->global_symbol_table, name);
  }
  return sym;
}

// Find a section by name.
ELFReaderSection* LinkerFileFindSection(LinkerFile* file, String* name) {
  return MapFind(&file->sections_by_name, name);
}


