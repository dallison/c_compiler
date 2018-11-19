//
//  loader.c
//
//  Created by David Allison on 1/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "loader.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

const bool kPrintSymbolTableOnStart = true;

Region* NewRegion(void* addr, int64_t length) {
  Region* region = malloc(sizeof(Region));
  region->address = addr;
  region->length = length;
  return region;
}

static Symbol* NewSymbol(ELFSymbol* elf_sym, const char* name, uint64_t address) {
  Symbol* symbol = malloc(sizeof(Symbol));
  symbol->header = elf_sym;
  StringInit(&symbol->name, name);
  symbol->address = address;
  return symbol;
}

static void SymbolDelete(Symbol* sym) {
  StringDestruct(&sym->name);
  free(sym);
}

//
// Symbol table.  This is a hash table of Vectors.  The Vectors
// contain Symbol pointers.  The key for the hash table is the address
// of the symbol.
//
static size_t SymbolHash(void* value, HashTable* table, HashMode mode) {
  uint64_t address;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer a Symbol.
      address = ((Symbol*)value)->address;
      break;
    case kHashSearch:
      // For search we have an address.
      // to find.
      address = (uint64_t)value;
      break;
  }
  return address;
}

static bool SymbolInsertInHashTable(void* entry, void* value, void** parent) {
  if (entry == NULL) {
    entry = NewVector();
    *parent = entry;
  }
  Vector* bucket = (Vector*)entry;
  VectorAppend(bucket, value);
  return true;
}

static void* SymbolFindInHashTable(void* entry, void* value) {
  if (entry == NULL) {
    return NULL;
  }
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* sym = bucket->value[i];
    if (sym->address == (uint64_t)value) {
      return sym;
    }
  }
  return NULL;
}

static void DeleteSymbolList(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* sym = bucket->value[i];
    SymbolDelete(sym);
  }
  VectorDelete(bucket);
}

static void ClearSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolList, NULL);
}


static void VLoaderError(const char* error, va_list ap) {
  vfprintf(stderr, error, ap);
}

static void LoaderError(const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLoaderError(error, ap);
  va_end(ap);
}



static void VLoaderWarning(const char* warn, const char* error, va_list ap) {
  vfprintf(stderr, error, ap);
}

static void LoaderWarning(const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLoaderWarning(warn, error, ap);
  va_end(ap);
  
}

Symbol* LoaderFindSymbol(Loader* loader,
                               uint64_t address) {
  return HashTableSearch(&loader->global_symbol_table, (void*)address);
}

static void InsertSymbol(HashTable* symbol_table,
                        Symbol* sym) {
  HashTableInsert(symbol_table, sym);
}

static void PrintSymbolList(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    Symbol* symbol = bucket->value[i];
    printf("0x%016llx: %s\n", symbol->address, symbol->name.value);
  }
}

// Print the symbol tables for debugging.
static void PrintSymbolTable(Loader* loader) {
  HashTableTraverse(&loader->global_symbol_table, PrintSymbolList, NULL);
}

static void ReadSymbol(Loader* loader,
                      ELFReaderFile* elf_file,
                      ELFReaderSection* symtab,
                      ELFReaderSection* strtab,
                      ELFSymbol* elf_sym) {
  if (elf_sym->name > strtab->header->size) {
    LoaderError("Corrupt symbol name");
    return;
  }
  const char* sym_name = (const char*)strtab->contents + elf_sym->name;
  uint64_t sym_address = elf_sym->value;
  
  // Symbol is new.  Add it to the global symbol table.
  Symbol* sym = NewSymbol(elf_sym, sym_name, sym_address);
  InsertSymbol(&loader->global_symbol_table, sym);
}


bool LoaderInitFromFile(Loader* loader, String* filename) {
  // Load the ELF file.
  loader->elf_file = NewELFReaderFile(filename);
  bool ok = ELFReaderFileRead(loader->elf_file, 0, 0);
  if (!ok) {
    return false;
  }

  // Initialize regions vector.
  VectorInit(&loader->regions);

  // Add the whole ELF file to the regions vector.
  VectorAppend(&loader->regions, NewRegion(loader->elf_file->header, loader->elf_file->file_length));

  // Get the entry point address.
  loader->main_address = loader->elf_file->header->entry;

  HashTableInit(&loader->global_symbol_table, "symbols", 111, SymbolHash, SymbolInsertInHashTable, SymbolFindInHashTable);
  
  // Read the symbol tables and insert all the symbols into the global symbol table.
  for (size_t i = 0; i < loader->elf_file->sections.length; i++) {
    ELFReaderSection* section = loader->elf_file->sections.value[i];
    if (section->header->type == SHT(symtab)) {
      ELFReaderSection* symtab = section;
      if (symtab->header->link >= loader->elf_file->sections.length) {
        LoaderError("Corrupt symbol table link value");
        continue;
      }
      ELFReaderSection* strtab = loader->elf_file->sections.value[symtab->header->link];
      size_t num_symbols = symtab->header->size / symtab->header->entsize;
      const char* symbol_addr = (const char*)loader->elf_file->header + symtab->header->offset;
      
      // Now read the symbols and add them to the symbol tables in the file.
      for (size_t i = 0; i < num_symbols; i++) {
        ELFSymbol* elf_sym = (ELFSymbol*)symbol_addr;
        ReadSymbol(loader, loader->elf_file, symtab, strtab, elf_sym);
        symbol_addr += symtab->header->entsize;
      }
    }
  }
  
  if (kPrintSymbolTableOnStart) {
    PrintSymbolTable(loader);
  }
  
  // Process all program segments and look for PT_LOAD types.  These are
  // loadable segments that have an address assigned to them.  We map them in
  // from the file directly using the 'mmap' function provided by the operating
  // system.  This allocates the virtual memory pages at the specified address
  // and points them to the file contents so that when they are read the page
  // is filled with the data in the file.
  for (size_t i = 0; i < loader->elf_file->segments.length; i++) {
    ELFProgramHeader* segment = loader->elf_file->segments.value[i];
    if (segment->type == PT(load)) {
      uint64_t addr = segment->vaddr;       // Address to place segment at.
      uint64_t load_addr = addr;
      uint64_t offset = segment->offset;    // Offset into file.

      // Align address and offset to lower page boundary.  The address and
      // file offset must be page-aligned for the mmap function to operate
      // correctly (it will error out if this is not the case).  We assume
      // our pages are 4K long as this is almost universally true these daye.
      addr &= ~0xfff;
      offset &= ~0xfff;

      // Protection for mmap and open.  We have to open the file in order the mmap it.
      // If the mapping is going to allow writes to the pages we need to open the file
      // in read-write mode, but we won't be writing to it.
      int prot = PROT_READ;
      int file_prot = O_RDONLY;
      if ((segment->flags & PF(w)) != 0) {
        // Segment is writeable.
        prot |= PROT_WRITE;
        file_prot = O_RDWR;
      }
      if ((segment->flags & PF(x)) != 0) {
        // Segment is executable.
        prot |= PROT_EXEC;
      }

      // Length of segment in memory.  We can map memory up the next
      // page boundary beyond the end of the file.  If the memsz is
      // beyond that we need to mmap a new ANON segment for it.
      int64_t length = segment->filesz + (segment->vaddr & 0xfff);
      length = (length + 0xfff) & ~0xfff;
      
      // Open the ELF file again to get a file descriptor that we can use
      // for mmap.
      int fd = open(filename->value, file_prot);
      if (fd < 0) {
        printf("Failed to open ELF file segment\n");
        return false;
      }

      if (length == 0) {
        continue;
      }
      
      // Map in the segment at the address specified in the ELF file.  This is done using
      // the MAP_PRIVATE flag so that the pages are all copy-on-write, meaning that they
      // will be copied to a new physical address if they are written to, otherwise they
      // are shared with other physical pages that map the same file in.  The MAP_FIXED
      // flag says that we are providing the address to map the pages at.  Normally mmap
      // chooses the address, but in this case we need them at the same virtual address
      // that he Loader chose and specified in the segment header.
      void* segment_ptr = mmap((void*)addr, length, prot, MAP_PRIVATE|MAP_FIXED, fd, offset);
      if (segment_ptr == MAP_FAILED) {
        printf("Failed to map in ELF segment: %s\n", strerror(errno));
        return false;
      }

      // Don't need the file descriptor now.
      close(fd);

      // Zero out any difference between memsz and filesz.  This will really only
      // be the .bss section.  We have mapped the contents of the file but some of
      // it will need to be zeroed out.  We also need to allocate a contiguous
      // anonymous region of zeros above the segment.
      // We need the .bss to be all zeroes before thae program starts.
      if ((prot & PROT_WRITE) != 0) {
        void* zeroed_region = (char*)load_addr + segment->filesz;   // Start of zero memory.
        int64_t zeroed_region_size = (char*)segment_ptr + length - (char*)zeroed_region;
        //length - segment->filesz;
        memset(zeroed_region, 0, zeroed_region_size);
        
        // Any additional memory beyond the file.
        int64_t additional_memory = segment->memsz - length;
        if (additional_memory > 0) {
          void* zero = (char*)segment_ptr + length;
          zero = mmap(zero, additional_memory, PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, 0, 0);
          if (zero == MAP_FAILED) {
            printf("Failed to map in ELF segment: %s\n", strerror(errno));
            return false;
          }
          VectorAppend(&loader->regions, NewRegion(zero, additional_memory));
        }
      }

      // Add a new region to the regions vector so that we can remove it
      // when destructed.
      VectorAppend(&loader->regions, NewRegion(segment_ptr, length));
    }
  }
  return true;
}

void LoaderDestruct(Loader* loader) {
  // Unmap all regions.
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value[i];
    munmap(region->address, region->length);
  }
  VectorDestructWithContents(&loader->regions, NULL);
  
  // Clear the symbol table and delete it.
  ClearSymbolTable(&loader->global_symbol_table);
  HashTableDestruct(&loader->global_symbol_table);
}
