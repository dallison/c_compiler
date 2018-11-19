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

// A Region is a mapped region of memory that should be unmapped when
// we are done.
typedef struct {
  void* address;        // Virtual address.
  int64_t length;       // Length in bytes.
} Region;

Region* NewRegion(void* addr, int64_t length);

typedef struct Symbol {
  ELFSymbol* header;
  String name;
  uint64_t address;
} Symbol;

// A loader loads ELF files into memory based on their contents.  The
// file header contains everything we need to find the loadable regions
// and load them into memory at the correct addresses.
typedef struct {
  ELFReaderFile* elf_file;
  uint64_t main_address;
  Vector regions;
  HashTable global_symbol_table;
} Loader;

bool LoaderInitFromFile(Loader* loader, String* filename);
void LoaderDestruct(Loader* loader);
Symbol* LoaderFindSymbol(Loader* loader,
                         uint64_t address);

#endif /* loader_h */
