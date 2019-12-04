//
//  loader.h
//
//  Created by David Allison on 1/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef loader_h
#define loader_h

#include "elf_reader.h"
#include <stdio.h>
#include "hashtable.h"
#include "map.h"
#include "loader_dynamic.h"
#include <stdarg.h>

struct LoaderArchitecture;

// A Region is a mapped region of memory that should be unmapped when
// we are done.
typedef struct {
  void* address;        // Virtual address.
  int64_t length;       // Length in bytes.
  Vector sections;      // Sections in this region (ELFReaderSection*).
} Region;

Region* NewRegion(void* addr, int64_t length);
void RegionDestruct(Region* region);

// A symbol scope is a region of memory that corresponds
// to a given symbol name.  This is used to cache a symbol
// lookup when inside a function where the symbol will not
// change until a call is made.
typedef struct {
  uint64_t start;
  uint64_t end;
  const char* name;
} SymbolScope;

// Loader flags.
#define LOADER_MAP_SYMTAB 1    // Load symbol table.
#define LOADER_LAZY_RESOLVE 2  // Use lazy PLT resolution.

// A loader loads ELF files into memory based on their contents.  The
// file header contains everything we need to find the loadable regions
// and load them into memory at the correct addresses.
typedef struct Loader {
  String filename;
  int32_t flags;
  struct LoaderArchitecture* arch;
  void* arch_data;
  String origin;
  String resolved_origin;
  
  ELFReaderFile* elf_file;
  uint64_t main_address;
  Vector regions;
  DynamicSection* dynamic;
  Vector library_search_path;
  LoadedDynamicLibrary* dynamic_lib;
  DynamicLibraryRegistry loaded_libraries;
  SymbolScope current_symbol;
} Loader;

bool LoaderInitFromFile(Loader* loader, String* filename,
                        int32_t flags,
                        struct LoaderArchitecture* arch, void* arch_data);
void LoaderDestruct(Loader* loader);
SymbolScope* LoaderFindSymbol(Loader* loader,
                         uint64_t address);
void LoaderSetCurrentSymbol(Loader* loader,
                          uint64_t address, uint64_t length, const char* name);
SymbolScope* LoaderGetCurrentSymbol(Loader* loader);

void LoaderError(const char* error, ...);

void VLoaderError(const char* error, va_list ap);

#endif /* loader_h */
