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
struct LinkerFile;

// A decoded ELFRelocation.
typedef struct {
  String symbol_name;
  ELFReaderSection* section;
  int type;
  int64_t offset;
  int32_t addend;
  int got_offset;     // Offset into GOT.
  int plt_offset;     // Offset into PLT.
} LinkerRelocation;

LinkerRelocation* NewLinkerRelocation(const char* symbol_name, int64_t offset,
                                      int32_t reloc_type);
void LinkerRelocationDestruct(LinkerRelocation* reloc);
void LinkerRelocationDelete(LinkerRelocation* reloc);

void LinkerReadRelocation(struct Linker* linker,
                    struct LinkerFile* file,
                    ELFReaderFile* elf_file,
                    ELFRelocation* reloc,
                    const char* symbol_table_address,
                    ELFReaderSection* reloc_section,
                    ELFReaderSection* symtab,
                    ELFReaderSection* strtab);

void LinkerApplyAllRelocations(struct Linker* linker);

#endif /* linker_reloc_h */
