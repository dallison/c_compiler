//
//  linker_dynamic.h
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//


#ifndef linker_dynamic_h
#define linker_dynamic_h

#include "vector.h"
#include <stdint.h>

struct LinkerSymbol;
struct Linker;
struct ELFWriterSectionContents;

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

// There are 2 parts to the GOT.  The first part contains the addresses
// of data and will be relocated at load time to contain the
// correct address.  This is used for access to global variables referenced
// from a DSO.
//
// The second part of the GOT is for the addresses of functions.  When loaded
// these are all set to an address that allows the dynamic linker to lookup
// the address of the function and set the GOT entry when the function
// is first called.
typedef struct {
  int num_reserved_entries;
  int entry_size;
  uint64_t address;
  Vector data_entries;     // Vector of pointers to LinkerSymbol.
  Vector function_entries; // Vector of pointers to LinkerSymbol.
} GlobalOffsetTable;

typedef struct {
  int num_reserved_entries;
  int entry_size;
  uint64_t address;
  Vector trampolines;
} ProcedureLinkageTable;


// TODO: think of a better name.  Maybe DynamicLinker?
typedef struct DynamicSection {
  GlobalOffsetTable global_offset_table;
  ProcedureLinkageTable procedure_linkage_table;
  Vector relocations;
  struct LinkerSymbol* global_offset_table_symbol;
  struct LinkerSymbol* dynamic_symbol;
} DynamicSection;

void DynamicSectionInit(DynamicSection* s, struct Linker* linker);
DynamicSection* NewDynamicSection(struct Linker* linker);
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

int GetDataGOTOffset(DynamicSection* s, struct LinkerSymbol* symbol);
int GetFunctionGOTOffset(DynamicSection* s, struct LinkerSymbol* symbol);
int GetPLTOffset(DynamicSection* s, struct LinkerSymbol* symbol);

//Section* DynamicInventSection(const char* name);

void BuildGlobalOffsetTable(struct Linker* linker, struct ELFWriterSectionContents* contents);

void GatherDynamicRelocations(struct Linker* linker);

#endif /* linker_dynamic_h */
