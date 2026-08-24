//
//  linker_reloc.h
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef linker_reloc_h
#define linker_reloc_h

#include "dstring.h"
#include "elf_reader.h"
#include <stdint.h>

struct Linker;
struct ObjectFile;
struct LinkerSymbol;

// A decoded ELFRelocation.
typedef struct Relocation {
  String symbol_name;        // LinkerSymbol name.
  struct LinkerSymbol* symbol;     // Decoded symbol (might be NULL).
  ELFReaderSection* symbol_section; // Target of an STT_SECTION relocation.
  uint64_t symbol_value;      // Value relative to symbol_section.
  ELFReaderSection* section; // Target section.
  int type;                  // Relocation type.
  int64_t offset;            // Offset into section.
  int64_t addend;            // Value to add to end.
} Relocation;

Relocation* NewRelocation(const char* symbol_name,
                            ELFReaderSection* target_section,
                            int64_t offset,
                            int32_t reloc_type,
                          int64_t addend);
Relocation* NewLinkerSymbolRelocation(struct LinkerSymbol* symbol,
                                      int64_t offset,
                                      int32_t reloc_type,
                                      int64_t addend);
Relocation* NewRelativeRelocation(struct LinkerSymbol* symbol,
                                  int64_t offset,
                                  ELFReaderSection* target_section,
                                  int32_t reloc_type,
                                  int64_t addend);
void RelocationDestruct(Relocation* reloc);
void RelocationDelete(Relocation* reloc);

void LinkerReadRelocation(struct Linker* linker,
                    struct ObjectFile* file,
                    ELFReaderFile* elf_file,
                    ELFRelocation* reloc,
                    const char* symbol_table_address,
                    ELFReaderSection* reloc_section,
                    ELFReaderSection* symtab,
                    ELFReaderSection* strtab);

void LinkerApplyAllRelocations(struct Linker* linker);

#endif /* linker_reloc_h */
