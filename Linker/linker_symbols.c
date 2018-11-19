//
//  linker_symbols.c
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_symbols.h"
#include "linker_file.h"
#include "linker.h"
#include <stdio.h>
#include <stdlib.h>

// Create a new Linker symbol based on an ELFSymbol in the ELF file.
LinkerSymbol* NewLinkerSymbol(ELFSymbol* elf_sym, LinkerFile* file) {
  LinkerSymbol* symbol = malloc(sizeof(LinkerSymbol));
  symbol->header = elf_sym;
  StringInit(&symbol->name, "");    // Name will be set later.
  symbol->defined = false;
  symbol->section = NULL;
  symbol->file = file;
  symbol->address = 0;
  symbol->size = elf_sym->size;
  symbol->delete_header = false;
  symbol->got_offset = -1;
  symbol->plt_offset = -1;
  return symbol;
}

void LinkerSymbolDelete(LinkerSymbol* sym) {
  StringDestruct(&sym->name);
  if (sym->delete_header) {
    free(sym->header);
  }
  free(sym);
}

//
// Symbol table.  This is a hash table of Vectors.  The Vectors
// contain LinkerSymbol pointers.
//
size_t LinkerSymbolHash(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer a LinkerSymbol.
      name = ((LinkerSymbol*)value)->name.value;
      break;
    case kHashSearch:
      // For search we have pointer to the name.
      // to find.
      name = (const char*)value;
      break;
  }
  size_t hash = 0;
  for (size_t i = 0; name[i] != '\0'; i++) {
    hash = (hash << 1) ^ name[i];
  }
  return hash;
}

bool LinkerSymbolInsertInHashTable(void* entry, void* value, void** parent) {
  if (entry == NULL) {
    entry = NewVector();
    *parent = entry;
  }
  Vector* bucket = (Vector*)entry;
  VectorAppend(bucket, value);
  return true;
}

void* LinkerSymbolFindInHashTable(void* entry, void* value) {
  if (entry == NULL) {
    return NULL;
  }
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value[i];
    if (StringEqual(&sym->name, (char*)value)) {
      return sym;
    }
  }
  return NULL;
}

static void DeleteSymbolList(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value[i];
    LinkerSymbolDelete(sym);
  }
  VectorDelete(bucket);
}

void LinkerClearSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolList, NULL);
}

static void PrintSymbolList(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value[i];
    const char* info = "";
    if (!symbol->defined) {
      info = "undefined";
    } else if (symbol->section != NULL) {
      info = symbol->section->name.value;
    }
    printf("0x%016llx: %-20s %s\n", symbol->address, symbol->name.value, info);
  }
}

static void PrintSymbolTable(HashTable* table) {
  HashTableTraverse(table, PrintSymbolList, NULL);
}

// Print the symbol tables for debugging.
void LinkerPrintSymbolTables(Linker* linker) {
  printf("Global symbol table\n");
  PrintSymbolTable(&linker->global_symbol_table);
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];
    printf("Local symbols in file %s\n", file->filename.value);
    PrintSymbolTable(&file->local_symbol_table);
  }
}

// Check if a symbol table bucket contains undefined symbola
// and if so, report them as errors.
static void CheckUndefined(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value[i];
    if (!symbol->defined) {
      LinkerError(symbol->file, "Undefined symbol %s", symbol->name.value);
    }
  }
}

// Check the global symbol table for undefined symbols.  There
// can't be any undefined local symbols.
void LinkerCheckForUndefinedSymbols(Linker* linker) {
  HashTableTraverse(&linker->global_symbol_table, CheckUndefined, NULL);
}

// Hash table traversal function to assign symbol addreses.
// The entry is a pointer to a vector of LinkerSymbol pointers.
static void AssignSymbolListAddresses(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value[i];
    ELFReaderSection* section = symbol->section;
    if (section != NULL) {
      symbol->address += section->address;
    }
  }
}

// Assign addresses to all defined symbols.
void LinkerAssignSymbolAddresses(Linker* linker) {
  // Assign all local symbol addresses.
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];
    HashTableTraverse(&file->local_symbol_table, AssignSymbolListAddresses, NULL);
  }

  // Assign global symbol addresses.
  HashTableTraverse(&linker->global_symbol_table, AssignSymbolListAddresses, NULL);
}

void LinkerAssignCommonSymbolAddresses(Linker* linker, uint64_t* address) {
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];
    for (size_t j = 0; j < file->common_symbols.length; j++) {
      LinkerSymbol* sym = file->common_symbols.value[j];
      sym->address = *address;
      *address += sym->size;
    }
  }
}

// Read a symbol from the ELF symbol table and add it to the
// linker's symbol tables.
void LinkerReadSymbol(Linker* linker,
                       LinkerFile* file,
                       ELFReaderFile* elf_file,
                       ELFReaderSection* symtab,
                       ELFReaderSection* strtab,
                       ELFSymbol* elf_sym) {
  int64_t info = elf_sym->info;
  int64_t binding = ELF_ST_BIND(info);
  if (elf_sym->name > strtab->header->size) {
    LinkerError(file, "Corrupt symbol name");
    return;
  }
  const char* sym_name = (const char*)strtab->contents + elf_sym->name;
  bool is_local_symbol = binding == STB(local);
  if (is_local_symbol) {
    // This is a local symbol.  It won't be present in the local
    // symbol table so we don't need to search for it.
    LinkerSymbol* sym = NewLinkerSymbol(elf_sym, file);
    StringSet(&sym->name, sym_name);

    // Assign symbol section, if it's not a reserved section index.
    if (elf_sym->shndx < SHN_LORESERVE) {
      int section_index = elf_sym->shndx;
      sym->section = elf_file->sections.value[section_index];
    } else if (elf_sym->shndx == SHN_COM) {
      // Common symbol, add to common_symbols vector.
      VectorAppend(&file->common_symbols, sym);
    }
    LinkerInsertSymbol(&file->local_symbol_table, sym);
    sym->defined = true;
    sym->address = elf_sym->value;
    return;
  }

  // Are we defining this symbol?
  bool defining_symbol = elf_sym->shndx != 0;

  // This is a global symbol.  It might already be present in the
  // the global symbol table.  See if it already exists.
  LinkerSymbol* sym = HashTableSearch(&linker->global_symbol_table,
                                      (void*)sym_name);
  if (sym != NULL) {
    // Symbol already exists.  Make sure this is not a duplicate definition.

    if (defining_symbol) {
      if (sym->defined) {
        if (sym->header->shndx == SHN_COM && elf_sym->shndx == SHN_COM) {
          // Both old and new are common symbols.  This is fine.
          // Common symbols are combined but the size is the biggest of
          // all versions of it.
          if (elf_sym->size > sym->size) {
            sym->size = elf_sym->size;
          }
          return;
        }
        // Symbol is already defined but we are redefining it.  This is an error.
        // TODO:
        LinkerError(file, "Multiple definition of symbol %s", sym->name.value);
        return;
      }

      // Assign the defining section to the symbol.
      if (elf_sym->shndx < SHN_LORESERVE) {
        int section_index = elf_sym->shndx;
        sym->section = elf_file->sections.value[section_index];
      }

      // This is now a defined symbol, so we can set the header to the definition.  Also
      // set the address.
      sym->header = elf_sym;
      sym->address = elf_sym->value;
      sym->defined = true;
    }
    return;
  }

  // Symbol is new.  Add it to the global symbol table.
  sym = NewLinkerSymbol(elf_sym, file);
  StringSet(&sym->name, sym_name);
  sym->defined = defining_symbol;
  sym->address = sym->header->value;

  // Assign symbol section, if it's not a reserved section index.
  if (elf_sym->shndx < SHN_LORESERVE) {
    int section_index = elf_sym->shndx;
    sym->section = elf_file->sections.value[section_index];
  } else if (elf_sym->shndx == SHN_COM) {
    // Record common symbol.
    VectorAppend(&file->common_symbols, sym);
  }

  LinkerInsertSymbol(&linker->global_symbol_table, sym);
}

LinkerSymbol* LinkerInventSymbol(Linker* linker, const char* name, int size) {
  ELFSymbol* elf_sym = malloc(sizeof(ELFSymbol));
  elf_sym->size = size;
  LinkerSymbol* sym = NewLinkerSymbol(elf_sym, NULL);
  LinkerInsertSymbol(&linker->global_symbol_table, sym);
  return sym;
}

// Map traversal function to assign the addresses to the section symbols.  This
// is called from the MapTraverse call in AssignSectionSymbolAddresses.
static void AssignSectionAddress(const void* key, void* value, void* data) {
  ELFReaderSection* section = value;
  LinkerFile* file = data;
  LinkerSymbol* section_symbol = LinkerFileFindSymbol(file, section->name.value);
  if (section_symbol != NULL) {
    section_symbol->address = section->address;
  }
}

// Go through all files and assign addresses to the section symbols.  There
// is a symbol for each section (with type SHT(section)) in each file.  The
// value of these symbols is the address of the start of the section.
void LinkerAssignSectionSymbolAddresses(Linker* linker) {
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];
    MapTraverse(&file->sections_by_name, AssignSectionAddress, file);
  }
}

// Go through each file and assign every common symbol to the NOBITS (bss)
// output section.  Each unique common symbol will have an address in the
// .bss section.  This section occupies no storage in the output ELF
// file but is allocated when the program is loaded.
void LinkerAssignBSSSymbolAddresses(Linker* linker) {
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];
    LinkerSymbol* section_symbol = LinkerFileFindSymbol(file, ".bss");
    if (section_symbol != NULL) {
      section_symbol->address = linker->nobits_address;
    }
  }
}

void LinkerDefineGlobalSymbol(Linker* linker, const char* name, ELFReaderSection* section,
                              int32_t type, int64_t size, int64_t value) {
  ELFSymbol* elf_sym = malloc(sizeof(ELFSymbol));
  elf_sym->info = STB(global) << 4 | type;
  elf_sym->size = size;
  LinkerSymbol* symbol = NewLinkerSymbol(elf_sym, NULL);
  StringSet(&symbol->name, name);
  symbol->section = section;
  symbol->delete_header = true;     // We own this and it needs to be deleted.
  symbol->defined = true;
  LinkerInsertSymbol(&linker->global_symbol_table, symbol);
}
