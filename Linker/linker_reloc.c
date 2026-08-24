//
//  linker_reloc.c
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_reloc.h"
#include "linker_file.h"
#include "linker.h"
#include "linker_arch.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "risc_v_machine.h"
#include "p_code_machine.h"
#include "linker_dynamic.h"
#include <inttypes.h>

Relocation* NewRelocation(const char* symbol_name,
                          ELFReaderSection* target_section,
                          int64_t offset,
                          int32_t reloc_type,
                          int64_t addend) {
  Relocation* reloc = malloc(sizeof(Relocation));
  StringInit(&reloc->symbol_name, symbol_name);
  reloc->symbol = NULL;
  reloc->symbol_section = NULL;
  reloc->symbol_value = 0;
  reloc->offset = offset;
  reloc->type = reloc_type;
  reloc->addend = addend;
  reloc->section = target_section;
  return reloc;
}

Relocation* NewLinkerSymbolRelocation(LinkerSymbol* symbol, int64_t offset,
                                      int32_t reloc_type, int64_t addend) {
  Relocation* reloc = malloc(sizeof(Relocation));
  StringInit(&reloc->symbol_name, symbol->name.value);
  reloc->symbol = symbol;
  reloc->symbol_section = NULL;
  reloc->symbol_value = 0;
  reloc->offset = offset;
  reloc->type = reloc_type;
  reloc->addend = 0;
  reloc->section = NULL;
  return reloc;
}

Relocation* NewRelativeRelocation(LinkerSymbol* symbol,
                                  int64_t offset,
                                  ELFReaderSection* target_section,
                                  int32_t reloc_type,
                                  int64_t addend) {
  Relocation* reloc = malloc(sizeof(Relocation));
   StringInit(&reloc->symbol_name, symbol != NULL ? symbol->name.value : NULL);
   reloc->symbol = symbol;
   reloc->symbol_section = NULL;
   reloc->symbol_value = 0;
   reloc->offset = offset;
   reloc->type = reloc_type;
   reloc->addend = addend;
   reloc->section = target_section;
   return reloc;
}
              
void RelocationDestruct(Relocation* reloc) {
  StringDestruct(&reloc->symbol_name);
}

void RelocationDelete(Relocation* reloc) {
  RelocationDestruct(reloc);
  free(reloc);
}

// Read a relocation from the ELF file and add it to the linker's
// relocation table.  A relocation is a modification to a piece of
// data in the ELF file.  Once the linker has determined the addresses
// of all symbols it can use those symbol values to modify the requested
// locations.  Typically these are references to external symbols whose
// addresses are not known at compile time.
void LinkerReadRelocation(Linker* linker,
                           ObjectFile* file,
                           ELFReaderFile* elf_file,
                           ELFRelocation* reloc,
                           const char* symbol_table_address,
                           ELFReaderSection* reloc_section,
                           ELFReaderSection* symtab,
                           ELFReaderSection* strtab) {
  int32_t symbol_index = ELF_R_SYM(reloc->info);
  int32_t reloc_type = ELF_R_TYPE(reloc->info);
  int64_t addend = reloc_section->header->type == SHT(rela) ? reloc->addend : 0;
  
  // Decode the referenced symbol from the on-disk symbol table.  For ELF32 the
  // on-disk symbol is narrower than the canonical struct, so decode it via the
  // format ops; the symbol is only used immediately (for its name).
  const char* sym_addr =
      symbol_table_address + symbol_index * symtab->header->entsize;
  ELFSymbol sym_storage;
  ELFSymbol* elf_sym;
  if (elf_file->ops->is_64_bit) {
    elf_sym = (ELFSymbol*)sym_addr;
  } else {
    elf_file->ops->ReadSymbol(&sym_storage, sym_addr);
    elf_sym = &sym_storage;
  }

  // Get the target section index from the info field in the section header.
  int target_section_index = reloc_section->header->info;
  if (target_section_index < 0 || target_section_index >= elf_file->sections.length) {
    LinkerWarning(file, "bad-rel-section",
                  "Bad info in relocation section %s (info: %d)",
                  reloc_section->name.value, target_section_index);
    return;

  }
  
  ELFReaderSection* target_section = elf_file->sections.value.p[target_section_index];
  // Create a Relocation object.
  Relocation* linker_reloc =
    NewRelocation((const char*)strtab->contents + elf_sym->name,
                target_section, reloc->offset, reloc_type, addend);
  if (ELF_ST_TYPE(elf_sym->info) == STT(section) &&
      elf_sym->shndx < elf_file->sections.length) {
    linker_reloc->symbol_section =
        elf_file->sections.value.p[elf_sym->shndx];
    linker_reloc->symbol_value = elf_sym->value;
  }

  // Add relocation to file relocations vector.
  VectorAppend(&file->relocations, linker_reloc);
}



// Apply a relocation.  A relocation targets a particular instruction or
// data in a section.
static void ApplyRelocation(Linker* linker, ObjectFile* file,
                            Relocation* reloc) {
  ELFReaderSection* target_section = reloc->section;
  if (target_section == NULL) {
    return;
  }
  
  // Find the value of the symbol to use.  This looks in the local symbol
  // table first, then the global symbol table.
  LinkerSymbol* symbol = NULL;
  uint64_t S;
  if (reloc->symbol_section != NULL) {
    S = reloc->symbol_section->address + reloc->symbol_value;
  } else {
    symbol = ObjectFileFindSymbol(file, reloc->symbol_name.value);
    S = symbol != NULL ? symbol->address : 0;
  }
  if (symbol == NULL && reloc->symbol_section == NULL) {
    // Trying to apply relocation for undefined symbol
    LinkerError(file, "Undefined symbol %s used in relocation", reloc->symbol_name.value);
    return;
  }

  // What address are we applying the relocation to.
  char* target_address = (char*)target_section->contents + reloc->offset;
  
  int64_t A = reloc->addend;      // Addend.
  

  if (linker->print_relocations) {
    printf("Applying relocation type %d for symbol %s(0x%" PRIx64 ") to offset %" PRId64 "\n",
         reloc->type,
         reloc->symbol_name.value,
         S, reloc->offset);
  }
  assert(linker->arch != NULL);
  linker->arch->apply_relocation(linker, file, reloc,
                                 symbol, target_address,
                                 S, A);
}

// Go through each file (each .o file) and apply the relocations contained
// in it to their target sections.
void LinkerApplyAllRelocations(Linker* linker) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    ObjectFile* file = linker->files.value.p[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      ApplyRelocation(linker, file, file->relocations.value.p[reloc_index]);
    }
  }
}

