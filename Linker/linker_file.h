//
//  linker_file.h
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef linker_file_h
#define linker_file_h

#include "elf_reader.h"
#include "map.h"
#include "hashtable.h"
#include "vector.h"
#include "linker_symbols.h"

typedef struct LinkerFile {
  String filename;
  ELFReaderFile* elf_file;
  HashTable local_symbol_table;
  Vector relocations;         // Vector of LinkerRelocation*.
  struct Linker* linker;
  Map sections_by_name;       // Map of section name vs ELFReaderSection* in elf_file.
  Map sections_by_type;       // Map of section type vs Vector of ELFReaderSection*
  Vector common_symbols;      // Vector of LinkerSymbol*.
} LinkerFile;

LinkerFile* NewLinkerFile(ELFReaderFile* elf_file, struct Linker* linker,
                          const char* filename);
void LinkerFileDestruct(LinkerFile* file);
void LinkerFileDelete(LinkerFile* file);
LinkerSymbol* LinkerFileFindSymbol(LinkerFile* file, const char *name);
ELFReaderSection* LinkerFileFindSection(LinkerFile* file, String* name);


#endif /* linker_file_h */
