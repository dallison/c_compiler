//
//  linker_symbols.h
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef linker_symbols_h
#define linker_symbols_h

#include <stdbool.h>
#include <stdint.h>
#include "elf.h"
#include "elf_reader.h"

struct Linker;
struct LinkerFile;

typedef struct LinkerSymbol {
  ELFSymbol* header;
  String name;
  bool defined;         // Symbol is defined.
  ELFReaderSection* section;    // Section containing symbol (or NULL for COM)
  struct LinkerFile* file;
  size_t size;          // Number of bytes in used by symbol;
  uint64_t address;     // Address assigned by linker.
  bool delete_header;   // The ELFHeader is owned (and needs deleted).
  int got_offset;       // Global Offset Table offset (-1 = none)
  int plt_offset;       // Procedure Linkage Table offset (-1 = none)
} LinkerSymbol;

LinkerSymbol* NewLinkerSymbol(ELFSymbol* elf_sym, struct LinkerFile* file);
void LinkerSymbolDelete(LinkerSymbol* sym);

size_t LinkerSymbolHash(void* value, HashTable* table, HashMode mode);
bool LinkerSymbolInsertInHashTable(void* entry, void* value, void** parent);
void* LinkerSymbolFindInHashTable(void* entry, void* value);

void LinkerPrintSymbolTables(struct Linker* linker);
void LinkerCheckForUndefinedSymbols(struct Linker* linker);
void LinkerAssignSymbolAddresses(struct Linker* linker);
void LinkerAssignCommonSymbolAddresses(struct Linker* linker, uint64_t* address);

void LinkerReadSymbol(struct Linker* linker,
                struct LinkerFile* file,
                ELFReaderFile* elf_file,
                ELFReaderSection* symtab,
                ELFReaderSection* strtab,
                ELFSymbol* elf_sym);
LinkerSymbol* LinkerInventSymbol(struct Linker* linker, const char* name, int size);

void LinkerAssignSectionSymbolAddresses(struct Linker* linker);
void LinkerAssignBSSSymbolAddresses(struct Linker* linker);
void LinkerDefineGlobalSymbol(struct Linker* linker, const char* name, ELFReaderSection* section,
                              int32_t type, int64_t size, int64_t value);
void LinkerClearSymbolTable(HashTable* table);

#endif /* linker_symbols_h */
