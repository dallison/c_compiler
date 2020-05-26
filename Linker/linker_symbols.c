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
Symbol* NewSymbol(ELFSymbol* elf_sym, ObjectFile* file) {
  Symbol* symbol = malloc(sizeof(Symbol));
  symbol->header = elf_sym;
  StringInit(&symbol->name, "");    // Name will be set later.
  symbol->defined = false;
  symbol->invented = false;
  symbol->section = NULL;
  symbol->file = file;
  symbol->address = 0;
  symbol->size = elf_sym->size;
  symbol->got_index = -1;
  symbol->plt_index = -1;
  symbol->index = -1;
  symbol->dynamic_index = -1;
  return symbol;
}

void SymbolDelete(Symbol* sym) {
  StringDestruct(&sym->name);
  free(sym);
}

//
// Symbol table.  This is a hash table of Vectors.  The Vectors
// contain Symbol pointers.
//
size_t SymbolHash(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer a Symbol.
      name = ((Symbol*)value)->name.value;
      break;
    case kHashSearch:
      // For search we have pointer to the name.
      // to find.
      name = (const char*)value;
      break;
  }
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
  }
  return hash;
}

bool SymbolInsertInHashTable(void* entry, void* value, void** parent) {
  if (entry == NULL) {
    entry = NewVector();
    *parent = entry;
  }
  Vector* bucket = (Vector*)entry;
  VectorAppend(bucket, value);
  return true;
}

void* SymbolFindInHashTable(void* entry, void* value) {
  if (entry == NULL) {
    return NULL;
  }
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* sym = bucket->value.p[i];
    if (StringEqual(&sym->name, (char*)value)) {
      return sym;
    }
  }
  return NULL;
}

static void DeleteSymbolList(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* sym = bucket->value.p[i];
    SymbolDelete(sym);
  }
  VectorDelete(bucket);
}

void LinkerClearSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolList, NULL);
}

static void PrintSymbolList(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* symbol = bucket->value.p[i];
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
    ObjectFile* file = linker->files.value.p[i];
    printf("Local symbols in file %s\n", file->filename.value);
    PrintSymbolTable(&file->local_symbol_table);
  }
}

// Check if a symbol table bucket contains undefined symbola
// and if so, report them as errors.
static void CheckUndefined(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* symbol = bucket->value.p[i];
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
// The entry is a pointer to a vector of Symbol pointers.
static void AssignSymbolListAddresses(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* symbol = bucket->value.p[i];
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
    ObjectFile* file = linker->files.value.p[i];
    HashTableTraverse(&file->local_symbol_table, AssignSymbolListAddresses, NULL);
  }

  // Assign global symbol addresses.
  HashTableTraverse(&linker->global_symbol_table, AssignSymbolListAddresses, NULL);
}

void LinkerAssignCommonSymbolAddresses(Linker* linker, uint64_t* address) {
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    for (size_t j = 0; j < file->common_symbols.length; j++) {
      Symbol* sym = file->common_symbols.value.p[j];
      sym->address = *address;
      *address += sym->size;
    }
  }
}

static void AssignSymbolSectionIndex(Symbol* sym,
                                     ObjectFile* file,
                                     ELFReaderFile* elf_file,
                                     ELFSymbol* elf_sym) {
  if (elf_sym->shndx != 0 && elf_sym->shndx < SHN_LORESERVE) {
    int section_index = elf_sym->shndx;
    sym->section = elf_file->sections.value.p[section_index];
  } else if (elf_sym->shndx == SHN_COM) {
    // Common symbol, add to common_symbols vector.
    VectorAppend(&file->common_symbols, sym);
  }
}

static void ReadLocalSymbol(const char* sym_name,
                            ObjectFile* file,
                            ELFReaderFile* elf_file,
                            ELFSymbol* elf_sym) {
  Symbol* sym = NewSymbol(elf_sym, file);
  StringSet(&sym->name, sym_name);
  
  // Assign symbol section, if it's not a reserved section index.
  AssignSymbolSectionIndex(sym, file, elf_file, elf_sym);
  
  // Insert symbol into local symbol table.
  LinkerInsertSymbol(&file->local_symbol_table, sym);
  sym->defined = true;
  sym->address = elf_sym->value;
}

static void RedefineSymbol(Symbol* sym,
                           ObjectFile* file,
                           ELFReaderFile* elf_file,
                           ELFSymbol* elf_sym) {
  // Are we defining this symbol?
  bool defining_symbol = elf_sym->shndx != 0;
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
      sym->section = elf_file->sections.value.p[section_index];
    }
    
    // This is now a defined symbol, so we can set the header to the definition.  Also
    // set the address.
    sym->header = elf_sym;
    sym->address = elf_sym->value;
    sym->defined = true;
  }
}

// Read a symbol from the ELF symbol table and add it to the
// linker's symbol tables.
void LinkerReadSymbol(Linker* linker,
                       ObjectFile* file,
                       ELFReaderFile* elf_file,
                       ELFReaderSection* strtab,
                       ELFSymbol* elf_sym) {
  int64_t info = elf_sym->info;
  int64_t binding = ELF_ST_BIND(info);
  if (elf_sym->name > strtab->header->size) {
    LinkerError(file, "Corrupt symbol name");
    return;
  }
  const char* sym_name = (const char*)strtab->contents + elf_sym->name;
  if (sym_name[0] == '\0') {
    // Don't insert empty symbol.
    return;
  }
  bool is_local_symbol = binding == STB(local);
  if (is_local_symbol) {
    // This is a local symbol.  It won't be present in the local
    // symbol table so we don't need to search for it.
    ReadLocalSymbol(sym_name, file, elf_file, elf_sym);
    return;
  }

  // This is a global symbol.  It might already be present in the
  // the global symbol table.  See if it already exists.
  Symbol* sym = LinkerFindSymbol(&linker->global_symbol_table,
                                      (void*)sym_name);
  if (sym != NULL) {
    // Symbol already exists.  Make sure this is not a duplicate definition.
    RedefineSymbol(sym, file, elf_file, elf_sym);
    return;
  }

  // Symbol is new.  Add it to the global symbol table.
  sym = NewSymbol(elf_sym, file);
  StringSet(&sym->name, sym_name);
  sym->defined = elf_sym->shndx != 0;
  sym->address = sym->header->value;

  // Assign symbol section, if it's not a reserved section index.
  AssignSymbolSectionIndex(sym, file, elf_file, elf_sym);

  // Insert symbol into global symbol table.
  LinkerInsertSymbol(&linker->global_symbol_table, sym);
}

Symbol* LinkerInventSymbol(Linker* linker, const char* name, int size) {
  ELFSymbol* elf_sym = malloc(sizeof(ELFSymbol));
  elf_sym->size = size;
  Symbol* sym = NewSymbol(elf_sym, NULL);
  sym->defined = true;
  sym->invented = true;
  // Name is not set in NewSymbol.  Normally it comes from the string
  // table but we don't have that for these names.
  StringSet(&sym->name, name);
  LinkerInsertSymbol(&linker->global_symbol_table, sym);
  return sym;
}

// Map traversal function to assign the addresses to the section symbols.  This
// is called from the MapTraverse call in AssignSectionSymbolAddresses.
static void AssignSectionAddress(MapKeyValue* kv, void* data) {
  ELFReaderSection* section = kv->value.p;
  ObjectFile* file = data;
  Symbol* section_symbol = ObjectFileFindSymbol(file, section->name.value);
  if (section_symbol != NULL) {
    section_symbol->address = section->address;
  }
}

// Go through all files and assign addresses to the section symbols.  There
// is a symbol for each section (with type SHT(section)) in each file.  The
// value of these symbols is the address of the start of the section.
void LinkerAssignSectionSymbolAddresses(Linker* linker) {
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    MapTraverse(&file->sections_by_name, AssignSectionAddress, file);
  }
}

// Go through each file and assign every common symbol to the NOBITS (bss)
// output section.  Each unique common symbol will have an address in the
// .bss section.  This section occupies no storage in the output ELF
// file but is allocated when the program is loaded.
void LinkerAssignBSSSymbolAddresses(Linker* linker) {
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    Symbol* section_symbol = ObjectFileFindSymbol(file, ".bss");
    if (section_symbol != NULL) {
      section_symbol->address = linker->nobits_address;
    }
  }
}


