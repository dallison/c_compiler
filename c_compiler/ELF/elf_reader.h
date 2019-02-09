//
//  elf_reader.h
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef elf_reader_h
#define elf_reader_h

// This is a general purpose ELF (Execuable and Linkable Format) file reader.

#include <stdio.h>
#include "elf.h"
#include "vector.h"
#include "dstring.h"
#include "hashtable.h"
#include "buffer.h"

typedef struct {
  ELFSectionHeader* header;
  String name;
  void* contents;               // Contents of section.
  uint64_t address;             // Address assigned to section.
  int32_t output_section_index; // Section index in output.
} ELFReaderSection;

ELFReaderSection* NewELFReaderSection(void);
void ELFReaderSectionDelete(ELFReaderSection* section);

typedef struct {
  ELFHeader* header;          // Header mapped from ELF file.
  String filename;
  int64_t file_length;
  Vector sections;            // Vector of ELFReaderSection*.
  Vector segments;            // Vector of ELFProgramHeader*.
  const char* section_names;  // Section names string table mapped from file.
} ELFReaderFile;

void ELFReaderFileInit(ELFReaderFile* elf, String* filename);
ELFReaderFile* NewELFReaderFile(String* filename);

bool ELFReaderFileRead(ELFReaderFile* elf, int64_t length, int64_t offset);
void ELFReaderFileDestruct(ELFReaderFile* elf);
void ELFReaderFileDelete(ELFReaderFile* elf);
ELFReaderSection* ELFReaderFileFindSection(ELFReaderFile* elf, const char* name);
void ELFReaderFileFindSectionsByType(ELFReaderFile* elf, int32_t type, Vector* output);

#endif /* elf_reader_h */
