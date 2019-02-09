//
//  elf_reader.c
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "elf_reader.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

ELFReaderSection* NewELFReaderSection() {
  ELFReaderSection* section = malloc(sizeof(ELFReaderSection));
  StringInit(&section->name, "");
  section->contents = NULL;
  section->address = 0;
  return section;
}

void ELFReaderSectionDelete(ELFReaderSection* section) {
  StringDestruct(&section->name);
}

void ELFReaderFileInit(ELFReaderFile* elf, String* filename) {
  StringInit(&elf->filename, filename->value);
  memset(&elf->header, 0, sizeof(elf->header));
  VectorInit(&elf->sections);
  VectorInit(&elf->segments);
  elf->section_names = NULL;
}

ELFReaderFile* NewELFReaderFile(String* filename) {
  ELFReaderFile* elf = malloc(sizeof(ELFReaderFile));
  ELFReaderFileInit(elf, filename);
  return elf;
}

// Read the contents of the ELF file mapped into memory at
// the address 'addr'.
bool ReadFileContents(ELFReaderFile* elf, void* addr, int64_t length) {
  elf->header = addr;
  elf->file_length = length;
  
  // Address of the header as char* so we can perform math on it.
  const char* header_addr = addr;
  
  // Check that this is an ELF file by looking at the magic number at the start
  // of the header.
  if (header_addr[EI_MAG0] != '\x7f' || header_addr[EI_MAG1] != 'E' || header_addr[EI_MAG2] != 'L' ||
      header_addr[EI_MAG3] != 'F') {
    return false;
  }
  
  // Read the section headers.
  // The first section header is at header->shoff bytes from the
  // file header.
  const char* section_header = header_addr + elf->header->shoff;
  for (int i = 0; i < elf->header->shnum; i++) {
    ELFReaderSection* section = NewELFReaderSection();
    section->header = (ELFSectionHeader*)section_header;
    VectorAppend(&elf->sections, section);
    section_header += sizeof(ELFSectionHeader);   // Next section header.
  }
  
  // Read the section name string table.
  ELFReaderSection* section_names = elf->sections.value[elf->header->shstrndx];
  elf->section_names = header_addr + section_names->header->offset;
  
  // Now that we have the section names in memory we can initialize
  // the sections.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value[i];
    StringSet(&section->name, elf->section_names + section->header->name);
    section->contents = (void*)(header_addr + section->header->offset);
  }
  
  // Read the program segments and insert them into the segments vector.
  const char* segment_header = header_addr + elf->header->phoff;
  for (int i = 0; i < elf->header->phnum; i++) {
    ELFProgramHeader* segment = (ELFProgramHeader*)segment_header;
    VectorAppend(&elf->segments, segment);
    segment_header += sizeof(ELFProgramHeader);   // Next segment header.
  }
  return true;
}

// Read and ELF file from disk.  This maps the file into memory using the
// UNIX 'mmap' call.  The 'mmap' function takes the file descriptor of an
// open file and maps its contents into the virtual memory of the process.
// The address is chosen by the memory manager.
bool ELFReaderFileRead(ELFReaderFile* elf, int64_t length, int64_t offset) {
  if (length == 0) {
    // First get the length of the file if needed.
    struct stat st;
    int e = stat(elf->filename.value, &st);
    if (e != 0) {
      return false;
    }
    length = st.st_size;
  }
  
  // Open the file.
  int fd = open(elf->filename.value, O_RDONLY);
  if (fd < 0) {
    return false;
  }

  // Offset must be page aligned.
  int page_size = (int)sysconf(_SC_PAGESIZE);
  int page_mask = page_size - 1;
  int64_t aligned_offset = offset & ~page_mask;
  int64_t addr_diff = offset & page_mask;
  int64_t full_length = length + addr_diff;
  
  // Map it into memory.
  void* addr = mmap(NULL, full_length, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd,
                    aligned_offset);
  if (addr == MAP_FAILED) {
    close(fd);
    return false;
  }

  // Now we have the file mapped into memory at address 'addr'.  We can
  // access this memory directly.  It is mapped read-only so we can't
  // write to it.  Writing to it would overwrite the file contents.
  void* start_addr = (char*)addr + addr_diff;
  bool ok = ReadFileContents(elf, start_addr, length);
  if (!ok) {
    munmap(addr, length);
  }
  close(fd);
  return true;
}

void ELFReaderFileDestruct(ELFReaderFile* elf) {
  VectorDestruct(&elf->sections);
  VectorDestruct(&elf->segments);
}

void ELFReaderFileDelete(ELFReaderFile* elf) {
  ELFReaderFileDestruct(elf);
  free(elf);
}

ELFReaderSection* ELFReaderFileFindSection(ELFReaderFile* elf, const char* name) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value[i];
    if (StringEqual(&section->name, name)) {
      return section;
    }
  }
  return NULL;
}

void ELFReaderFileFindSectionsByType(ELFReaderFile* elf, int32_t type, Vector* output) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value[i];
    if (section->header->type == type) {
      VectorAppend(output, section);
    }
  }
}
