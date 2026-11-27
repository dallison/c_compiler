//
//  elf_reader.c
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "elf_reader.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

ELFReaderSection* NewELFReaderSection() {
  ELFReaderSection* section = malloc(sizeof(ELFReaderSection));
  StringInit(&section->name, "");
  section->contents = NULL;
  section->address = 0;
  section->offset = 0;
  section->output_section_index = 0;
  section->section_symbol_index = -1;
  section->discarded = false;
  VectorInit(&section->gc_refs);
  return section;
}

void ELFReaderSectionDelete(ELFReaderSection* section) {
  StringDestruct(&section->name);
  VectorDestruct(&section->gc_refs);
}

void ELFReaderFileInit(ELFReaderFile* elf, String* filename) {
  StringInit(&elf->filename, filename->value);
  elf->header = NULL;
  VectorInit(&elf->sections);
  VectorInit(&elf->segments);
  elf->section_names = NULL;
  elf->ops = NULL;
  elf->base = NULL;
  elf->owns_decoded = false;
}

ELFReaderFile* NewELFReaderFile(String* filename) {
  ELFReaderFile* elf = malloc(sizeof(ELFReaderFile));
  ELFReaderFileInit(elf, filename);
  return elf;
}

// Read the contents of the ELF file mapped into memory at
// the address 'addr'.
bool ReadFileContents(ELFReaderFile* elf, void* addr, int64_t length) {
  elf->file_length = length;
  elf->base = addr;

  // Address of the header as char* so we can perform math on it.
  const char* header_addr = addr;
  
  // Check that this is an ELF file by looking at the magic number at the start
  // of the header.
  if (header_addr[EI_MAG0] != '\x7f' || header_addr[EI_MAG1] != 'E' || header_addr[EI_MAG2] != 'L' ||
      header_addr[EI_MAG3] != 'F') {
    return false;
  }

  // Select the format operations from the file's class byte.
  bool is_64_bit =
      (uint8_t)header_addr[EI_CLASS] != ELFCLASS32;  // default to 64-bit.
  elf->ops = ELFFormatOpsFor(is_64_bit);

  // For ELF64 the on-disk layout matches the canonical (wide) in-memory
  // layout, so the header and section/segment headers can be referenced
  // directly from the mapping.  For ELF32 they have to be decoded into owned
  // wide structures.
  elf->owns_decoded = !is_64_bit;

  // Read the file header.
  if (elf->owns_decoded) {
    ELFHeader* header = malloc(sizeof(ELFHeader));
    elf->ops->ReadHeader(header, header_addr);
    elf->header = header;
  } else {
    elf->header = (ELFHeader*)addr;
  }

  // Read the section headers.
  // The first section header is at header->shoff bytes from the
  // file header.
  const char* section_header = header_addr + elf->header->shoff;
  for (int i = 0; i < elf->header->shnum; i++) {
    ELFReaderSection* section = NewELFReaderSection();
    if (elf->owns_decoded) {
      ELFSectionHeader* hdr = malloc(sizeof(ELFSectionHeader));
      elf->ops->ReadSectionHeader(hdr, section_header);
      section->header = hdr;
    } else {
      section->header = (ELFSectionHeader*)section_header;
    }
    VectorAppend(&elf->sections, section);
    section_header += elf->ops->section_header_size;   // Next section header.
  }
  
  // Read the section name string table.
  ELFReaderSection* section_names = elf->sections.value.p[elf->header->shstrndx];
  elf->section_names = header_addr + section_names->header->offset;
  
  // Now that we have the section names in memory we can initialize
  // the sections.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value.p[i];
    StringSet(&section->name, elf->section_names + section->header->name);
    section->contents = (void*)(header_addr + section->header->offset);
  }
  
  // Read the program segments and insert them into the segments vector.
  const char* segment_header = header_addr + elf->header->phoff;
  for (int i = 0; i < elf->header->phnum; i++) {
    if (elf->owns_decoded) {
      ELFProgramHeader* segment = malloc(sizeof(ELFProgramHeader));
      elf->ops->ReadProgramHeader(segment, segment_header);
      VectorAppend(&elf->segments, segment);
    } else {
      ELFProgramHeader* segment = (ELFProgramHeader*)segment_header;
      VectorAppend(&elf->segments, segment);
    }
    segment_header += elf->ops->program_header_size;   // Next segment header.
  }
  return true;
}

// Read an ELF file (or archive member) into a heap buffer.  DaveOS file
// MAP_PRIVATE is eager-fill and has been a source of linker faults; malloc +
// pread is correct on every host and keeps section pointers stable for the
// link.
bool ELFReaderFileRead(ELFReaderFile* elf, int64_t length, int64_t offset) {
  if (length == 0) {
    // First get the length of the file if needed.
    int e = stat(elf->filename.value, &elf->file_stat);
    if (e != 0) {
      return false;
    }
    length = elf->file_stat.st_size;
  }

  if (length < 0) {
    return false;
  }

  int fd = open(elf->filename.value, O_RDONLY);
  if (fd < 0) {
    return false;
  }

  void* addr = malloc((size_t)length);
  if (addr == NULL) {
    close(fd);
    return false;
  }

  size_t got = 0;
  while (got < (size_t)length) {
    ssize_t n = pread(fd, (char*)addr + got, (size_t)length - got,
                      (off_t)(offset + (int64_t)got));
    if (n <= 0) {
      free(addr);
      close(fd);
      return false;
    }
    got += (size_t)n;
  }
  close(fd);

  bool ok = ReadFileContents(elf, addr, length);
  if (!ok) {
    free(addr);
  }
  return ok;
}

void ELFReaderFileDestruct(ELFReaderFile* elf) {
  // For ELF32 the header and the per-section/segment headers were decoded into
  // heap-allocated wide structures, so free them here.  For ELF64 they point
  // directly into the file mapping and are not owned.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value.p[i];
    if (elf->owns_decoded) {
      free(section->header);
    }
    ELFReaderSectionDelete(section);
  }
  if (elf->owns_decoded) {
    for (size_t i = 0; i < elf->segments.length; i++) {
      free(elf->segments.value.p[i]);
    }
    free(elf->header);
    elf->header = NULL;
  }
  VectorDestruct(&elf->sections);
  VectorDestruct(&elf->segments);
}

void ELFReaderFileDelete(ELFReaderFile* elf) {
  ELFReaderFileDestruct(elf);
  free(elf);
}

ELFReaderSection* ELFReaderFileFindSection(ELFReaderFile* elf, const char* name) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value.p[i];
    if (StringEqual(&section->name, name)) {
      return section;
    }
  }
  return NULL;
}

void ELFReaderFileFindSectionsByType(ELFReaderFile* elf, int32_t type, Vector* output) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value.p[i];
    if (section->header->type == type) {
      VectorAppend(output, section);
    }
  }
}
