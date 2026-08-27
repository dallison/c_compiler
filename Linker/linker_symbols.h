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
struct ObjectFile;

typedef struct LinkerSymbol {
  ELFSymbol* header;
  String name;
  bool defined;         // LinkerSymbol is defined.
  // Satisfied by a shared object rather than by anything in this link, so its
  // address is not known until the loader resolves it.  This is not the same as
  // having no section: a COMMON symbol has none either, but the linker does give
  // it an address.
  bool from_dynamic_library;
  bool invented;
  ELFReaderSection* section;    // Section containing symbol (or NULL for COM)
  struct ObjectFile* file;
  size_t size;          // Number of bytes in used by symbol;
  uint64_t address;     // Address assigned by linker.
  int got_index;       // Global Offset Table index (-1 = none)
  int plt_index;       // Procedure Linkage Table index (-1 = none)
  int index;            // Index into symbol table.
  int dynamic_index;    // Index into dynamic symbol table.
} LinkerSymbol;

LinkerSymbol* NewLinkerSymbol(ELFSymbol* elf_sym, struct ObjectFile* file);
void LinkerSymbolDelete(LinkerSymbol* sym);
bool LinkerSymbolIsWeak(LinkerSymbol* sym);
bool LinkerSymbolIsUndefinedWeak(LinkerSymbol* sym);

size_t LinkerSymbolHash(void* value, HashTable* table, HashMode mode);
bool LinkerSymbolInsertInHashTable(void* entry, void* value, void** parent);
void* LinkerSymbolFindInHashTable(void* entry, void* value);

void LinkerPrintSymbolTables(struct Linker* linker);
void LinkerCheckForUndefinedSymbols(struct Linker* linker);
void LinkerAssignSymbolAddresses(struct Linker* linker);
void LinkerAssignCommonSymbolAddresses(struct Linker* linker, uint64_t* address);

void LinkerReadSymbol(struct Linker* linker,
                struct ObjectFile* file,
                ELFReaderFile* elf_file,
                ELFReaderSection* strtab,
                ELFSymbol* elf_sym);
LinkerSymbol* LinkerInventSymbol(struct Linker* linker, const char* name, int size);

void LinkerAssignSectionSymbolAddresses(struct Linker* linker);
void LinkerAssignBSSSymbolAddresses(struct Linker* linker);
void LinkerClearSymbolTable(HashTable* table);

#endif /* linker_symbols_h */
