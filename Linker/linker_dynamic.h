//
//  linker_dynamic.h
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "vector.h"

#ifndef linker_dynamic_h
#define linker_dynamic_h

struct LinkerSymbol;
struct Linker;

// Dynamic Shared Object (DSO)

// A DSO can be loaded at runtime to provide shared libraries.
// Each .so file contains:
// 1. A symbol table containing the exported and imported symbols in
//    the library
// 2. A Global Offset Table (GOT) consisting of the addresses of every
//    external variable used or defined by the library.  This is held
//    in read/write memory and the addresses are set when the file is loaded.
// 3. A Procedure Linkage Table (PLT) containing trampolines for each
//    function called inside the library.  The PLT entries are set to the
//    real address of the function upon load or when the procedure is first
//    called.
// 4. Relocations to lookup the addresses of the symbols in the GOT and
//    assign them to the entry.

typedef struct DynamicSection {
  Vector global_offset_table;   // Vector of pointers to LinkerSymbol.
  Vector procedure_linkage_table;
  Vector relocations;
  struct LinkerSymbol* global_offset_table_symbol;
  struct LinkerSymbol* dynamic_symbol;
} DynamicSection;

void DynamicSectionInit(DynamicSection* s);
DynamicSection* NewDynamicSection(void);
void DynamicSectionDestruct(DynamicSection* s);
void DynamicSectionDelete(DynamicSection* s);
void DynamicSectionInventSymbols(struct Linker* linker, DynamicSection* s);

typedef struct {
  struct LinkerFile* file;
  DynamicSection dynamic;
} DynamicLibrary;

DynamicLibrary* NewDynamicLibrary(struct LinkerFile* file);
void DynamicLibraryDestruct(DynamicLibrary* lib);
void DynamicLibraryDelete(DynamicLibrary* lib);

int GetGOTOffset(DynamicSection* s, struct LinkerSymbol* symbol, int reloc_type);
int GetPLTOffset(DynamicSection* s, struct LinkerSymbol* symbol, int reloc_type);

//Section* DynamicInventSection(const char* name);

#endif /* linker_dynamic_h */
