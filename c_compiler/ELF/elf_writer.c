//
//  elf_writer.c
//  c_compiler
//
//  Created by David Allison on 1/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "elf_writer.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vector.h"

void ELFWriterFileInit(ELFWriterFile* elf, ELFType type, int machine,
                       int flags, DynamicCallback dynamic_callback,
                       bool is_64_bit,
                       bool is_little_endian) {
  elf->ops = ELFFormatOpsFor(is_64_bit);
  memset(&elf->header, 0, sizeof(elf->header));
  elf->header.ident[EI_MAG0] = '\x7f';
  elf->header.ident[EI_MAG1] = 'E';
  elf->header.ident[EI_MAG2] = 'L';
  elf->header.ident[EI_MAG3] = 'F';
  elf->header.ident[EI_CLASS] = is_64_bit ? ELFCLASS64 : ELFCLASS32;
  elf->header.ident[EI_DATA] = is_little_endian ? 1 : 2;
  elf->header.ident[EI_VERSION] = 1;
  elf->header.ident[EI_OSABI] = 0;
  elf->header.type = type;
  elf->header.machine = machine;
  elf->header.flags = flags;
  elf->header.version = 1;
  elf->header.ehsize = (ELF_Half)elf->ops->header_size;
  elf->header.shentsize = (ELF_Half)elf->ops->section_header_size;
  elf->header.phentsize = (ELF_Half)elf->ops->program_header_size;
  // Program segment headers are immediately after file header.
  elf->header.phoff = elf->ops->header_size;
  VectorInit(&elf->sections);
  VectorInit(&elf->segments);
  BufferInit(&elf->string_table);
  VectorInit(&elf->symbol_table);
  BufferInit(&elf->section_names);
  VectorInit(&elf->section_fixups);
  elf->last_local_symbol_index = -1;
  
  // Add first (empty) symbol to the symbol table.  All fields
  // of this symbol are zero.
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  VectorAppend(&elf->symbol_table, sym);
  
  // The first valid string table index is 1, so add a zero
  // to the start of it.
  BufferAppend(&elf->string_table, "", 1);
  BufferAppend(&elf->section_names, "", 1);
  elf->dynamic_callback = dynamic_callback;
}

void ELFWriterFileDestruct(ELFWriterFile* elf) {
  VectorDestructWithContents(&elf->sections,
                             (VectorElementDestructor)ELFWriterSectionDestruct, /*free_element=*/true);
  VectorDestructWithContents(&elf->segments,
                             (VectorElementDestructor)ELFWriterSegmentDestruct, /*free_element=*/true);
  BufferDestruct(&elf->string_table);
  VectorDestructWithContents(&elf->symbol_table, NULL, /*free_element=*/true);
  BufferDestruct(&elf->section_names);
  VectorDestructWithContents(&elf->section_fixups, NULL, /*free_element=*/true);
}

void ELFWriterAddSectionFixupByName(ELFWriterFile* elf,
                                    ELFWriterFixupField field,
                              const char* from, const char* to) {
  ELFWriterSectionFixup* fixup = malloc(sizeof(ELFWriterSectionFixup));
  fixup->field = field;
  fixup->from = from;
  fixup->to = to;
  VectorAppend(&elf->section_fixups, fixup);
}

void ELFWriterAddSectionFixup(ELFWriterFile* elf,
                              ELFWriterFixupField field,
                              ELFWriterSection* from, ELFWriterSection* to) {
  ELFWriterAddSectionFixupByName(elf, field, from->name.value, to->name.value);
}

void ELFWriterFixupSections(ELFWriterFile* elf) {
  for (size_t i = 0; i < elf->section_fixups.length; i++) {
    ELFWriterSectionFixup* fixup = elf->section_fixups.value.p[i];
    ELFWriterSection* from = ELFWriterFindSection(elf, fixup->from);
    ELFWriterSection* to = ELFWriterFindSection(elf, fixup->to);
    assert(from != NULL && to != NULL);
    switch (fixup->field) {
      case kFixupFieldInfo:
        from->header.info = to->index;
        break;
      case kFixupFieldLink:
        from->header.link = to->index;
        break;
    }
  }
}

void ELFWriterSectionContentsInit(ELFWriterSectionContents* contents, ELFWriterSectionContentsDataLocation location) {
  contents->data_location = location;
  BufferInit(&contents->data.buffered);
  contents->size = 0;
}

ELFWriterSectionContents* NewELFWriterSectionContents(ELFWriterSectionContentsDataLocation location) {
  ELFWriterSectionContents* contents = malloc(sizeof(ELFWriterSectionContents));
  ELFWriterSectionContentsInit(contents, location);
  switch (location) {
    case kSectionContentsRaw:
      contents->data.raw = NULL;
      break;
    case kSectionContentsMulti:
      VectorInit(&contents->data.multi);
      break;
    case kSectionContentsBuffered:
      BufferInit(&contents->data.buffered);
      break;
    case kSectionContentsNobits:
    case kSectionContentsPad:
      break;
  }
  return contents;
}

void ELFWriterSectionContentsDestruct(ELFWriterSectionContents* contents) {
  switch (contents->data_location) {
    case kSectionContentsRaw:
      break;
    case kSectionContentsMulti:
      VectorDestruct(&contents->data.multi);
      break;
    case kSectionContentsBuffered:
      BufferDestruct(&contents->data.buffered);
      break;
    case kSectionContentsNobits:
    case kSectionContentsPad:
      break;
  }
}

static ELF_Word AddBufferedString(Buffer* buffer, String* str) {
  ELF_Word offset = (ELF_Word)buffer->length;
  BufferAppend(buffer, str->value, str->length + 1);
  return offset;
}


size_t ELFWriterSectionContentsGetLength(ELFWriterSectionContents* contents) {
  switch (contents->data_location) {
    case kSectionContentsBuffered:
      return contents->data.buffered.length;
    case kSectionContentsRaw:
      return contents->size;
    case kSectionContentsMulti: {
      size_t size = 0;
      for (size_t i = 0; i < contents->data.multi.length; i++) {
        size += ELFWriterSectionContentsGetLength(contents->data.multi.value.p[i]);
      }
      return size;
    }
    case kSectionContentsNobits:
    case kSectionContentsPad:
      return contents->size;
  }
}

void ELFWriterSectionContentsWrite(ELFWriterSectionContents* contents, FILE* fp) {
  switch (contents->data_location) {
    case kSectionContentsBuffered:
      fwrite(contents->data.buffered.value,
             contents->data.buffered.length, 1, fp);
      break;
    case kSectionContentsRaw:
      fwrite(contents->data.raw, contents->size, 1, fp);
      break;
    case kSectionContentsMulti: {
      for (size_t i = 0; i < contents->data.multi.length; i++) {
        ELFWriterSectionContentsWrite(contents->data.multi.value.p[i], fp);
      }
      break;
    }
    case kSectionContentsNobits:
      break;
    case kSectionContentsPad:
      for (size_t i = 0; i < contents->size; i++) {
        fputc(0, fp);
      }
      break;
  }
}

// Find a section given its name.
ELFWriterSection* ELFWriterFindSection(ELFWriterFile* elf, const char* name) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFWriterSection* section= elf->sections.value.p[i];
    if (StringEqual(&section->name, name)) {
      return section;
    }
  }
  return NULL;
}

// Add relocation sections for all relocations.
static void CreateRelocationSections(ELFWriterFile* elf,
                           Vector* relocation_sections,
                           ELFWriterSection* symtab) {
  // Record current section length because we will be adding sections
  // to the sections vector in the loop.
  size_t num_sections =  elf->sections.length;
  for (size_t i = 0; i < num_sections; i++) {
    ELFWriterSection* section = elf->sections.value.p[i];
    if (section->relocations != NULL && section->relocations->length != 0) {
      String relocation_section_name;
      // Invent relocation section name.
      StringInit(&relocation_section_name, "");
      StringPrintf(&relocation_section_name, ".rela%s", section->name.value);
    
      // Add relocation section after section it refers to.
      ELFWriterSection* reloc_sect = ELFWriterAddStandardSection(elf, relocation_section_name.value,
                                                              SHT(rela), 0);
      // All relocation sections link to the symtab section so that
      // they can find the symbol.
      ELFWriterAddSectionFixup(elf, kFixupFieldLink, reloc_sect, symtab);
      ELFWriterAddSectionFixup(elf, kFixupFieldInfo, reloc_sect, section);
      
      // RELA sections have a fixed entry size.
      reloc_sect->header.entsize = elf->ops->relocation_size;
      // Take ownership of the relocations.
      reloc_sect->relocations = section->relocations;
      section->relocations = NULL;
      
      // Keep track of the relocation section.  It now owns
      // all the relocations so we know what its size and
      // contents are.
      VectorAppend(relocation_sections, reloc_sect);
      StringDestruct(&relocation_section_name);
    }
  }
}

// Align v to power of 2.
static uint64_t AlignTo(uint64_t v, uint64_t p2) {
  return (v + (p2 - 1)) & ~(p2 - 1);
}

// Write padding to the file.
static void Pad(uint64_t size, FILE* fp) {
  const uint64_t kBufferSize = 4096;
  char buf[kBufferSize];
  memset(buf, 0xda, kBufferSize);
  while (size > 0) {
    size_t len = size > kBufferSize ? kBufferSize : size;
    fwrite(buf, len, 1, fp);
    size -= len;
  }
}

// Write all section headers to the given file.
static void WriteSectionHeaders(ELFWriterFile* elf,
                                   ELFWriterSection* symtab,
                                   ELFWriterSection* strtab,
                                   ELFWriterSection* shstrtab,
                                   Vector* relocation_sections,
                                   size_t num_segments,
                                   FILE* fp) {
  // Calculate the file offset for the first section data.   This is
  // incremented during the offset assignment loop below to be the
  // address of the next section's file offset.
  int64_t next_section_data_offset = elf->ops->header_size +
    num_segments * elf->ops->program_header_size +
    elf->sections.length * elf->ops->section_header_size;
  ELFWriterSection* prev = NULL;
  
  // Write all the section headers to the file.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFWriterSection* section = elf->sections.value.p[i];
    size_t data_length = 0;
    if (section->contents != NULL) {
      // Get section data length.
      data_length = ELFWriterSectionContentsGetLength(section->contents);
    } else {
      if (section == symtab) {
        data_length = elf->symbol_table.length * elf->ops->symbol_size;
      } else if (section == strtab) {
        data_length = elf->string_table.length;
      } else if (section == shstrtab) {
        data_length = elf->section_names.length;
      } else {
        bool section_found = false;
        for (size_t j = 0; j < relocation_sections->length; j++) {
          if (relocation_sections->value.p[j] == section) {
            data_length = section->relocations->length * elf->ops->relocation_size;
            section_found = true;
            break;
          }
        }
        (void)section_found;
        assert(section_found);
      }
    }
    
    uint64_t padding = 0;
    if (section->header.name != 0) {
      if (section->header.type == SHT(nobits)) {
        // A nobits section (e.g. .bss) occupies no space in the file, so it
        // must not introduce any alignment padding into the file: its file
        // offset is just the current position and the following section
        // handles its own alignment.  (Previously the alignment padding for
        // a nobits section was written but not accounted for in the data
        // offset, which shifted all following sections.  This only mattered
        // when the section data did not start on an aligned boundary, as
        // happens for ELF32 with its 52-byte header.)
        section->header.offset = next_section_data_offset;
      } else {
        uint64_t aligned_address = AlignTo(next_section_data_offset,
                                           section->header.addralign);
        // Padding to get to next section.
        padding = aligned_address - next_section_data_offset;
        section->header.offset = aligned_address;
        if (prev != NULL) {
           prev->padding = padding;
        }
      }
   }
    section->header.size = data_length;
    
    elf->ops->WriteSectionHeader(&section->header, fp);
    if (section->header.type != SHT(nobits)) {
      next_section_data_offset += data_length + padding;
    }
    
    
    // Record previous.
    prev = section;
  }
}

static void WriteSectionContents(ELFWriterFile* elf,
                             ELFWriterSection* symtab,
                             ELFWriterSection* strtab,
                             ELFWriterSection* shstrtab,
                             Vector* relocation_sections,
                             FILE* fp) {
  // Write the section data to the file.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFWriterSection* section = elf->sections.value.p[i];
    size_t data_length = 0;
    if (section->contents != NULL) {
      ELFWriterSectionContentsWrite(section->contents, fp);
    } else {
      if (section == symtab) {
        for (size_t sym_index = 0; sym_index < elf->symbol_table.length; sym_index++) {
          ELFSymbol* sym = elf->symbol_table.value.p[sym_index];
          elf->ops->WriteSymbol(sym, fp);
        }
      } else if (section == strtab) {
        data_length = elf->string_table.length;
        fwrite(elf->string_table.value, data_length, 1, fp);
      } else if (section == shstrtab) {
        data_length = elf->section_names.length;
        fwrite(elf->section_names.value, data_length, 1, fp);
      } else {
        bool section_found = false;
        // Write out the relocation section contents.
        for (size_t j = 0; j < relocation_sections->length; j++) {
          if (relocation_sections->value.p[j] == section) {
            for (size_t reloc_index = 0;
                 reloc_index < section->relocations->length; reloc_index++) {
              ELFRelocation* reloc = section->relocations->value.p[reloc_index];
              elf->ops->WriteRelocation(reloc, fp);
            }
            section_found = true;
            break;
          }
        }
        (void)section_found;
        assert(section_found);
      }
    }
        // Pad to next section address.
    if (section->padding != 0) {
      Pad(section->padding, fp);
    }
  }
}

static void WriteProgramHeaders(ELFWriterFile* elf, size_t num_segments,
                                FILE* fp) {
  // Handle segments.  These have references to sections which, by this point,
  // will have been given addresses and offsets.
  if (elf->segments.length > 0) {
    // The segments are just written after the file header.
    elf->header.phnum = num_segments;
    fseek(fp, elf->header.phoff, SEEK_SET);
    
    // Add a PHDR segment as the first segment.
    ELFProgramHeader phdr;
    memset(&phdr, 0, sizeof(phdr));
    phdr.offset = elf->header.phoff;
    phdr.filesz = num_segments * elf->ops->program_header_size;
    phdr.memsz = phdr.filesz;
    phdr.align = 8;
    phdr.flags = PF(r) | PF(x);
    phdr.type = PT(phdr);
    
    ELFWriterSegment* code_segment = elf->segments.value.p[0];
    ELFWriterSection* section0 = code_segment->sections.value.p[0];
    phdr.vaddr = (section0->address & ~(code_segment->header.align - 1)) +
              elf->header.phoff;
    phdr.paddr = phdr.vaddr;
    elf->ops->WriteProgramHeader(&phdr, fp);
    
    for (size_t i = 0; i < elf->segments.length; i++) {
      ELFWriterSegment* segment = elf->segments.value.p[i];
      bool start_address_assigned = false;
      for (size_t j = 0; j < segment->sections.length; j++) {
        ELFWriterSection* section = segment->sections.value.p[j];
        
        // Add section size to the memory size.
        segment->header.memsz += section->header.size;
        
        // If the section is present in the file (not NOBITS), add its
        // size to the filesz.
        if (section->header.type != SHT(nobits)) {
          segment->header.filesz += section->header.size;
        }
        
        // The first section gives us the offset and addresses for the segment.
        if (!start_address_assigned) {
          segment->header.offset = section->header.offset;
          segment->header.vaddr = section->address;
          segment->header.paddr = segment->header.vaddr;
          start_address_assigned = true;
        }
      }
      
      elf->ops->WriteProgramHeader(&segment->header, fp);
    }
  } else {
    // No segments, need to zero out the phoff
    elf->header.phoff = 0;
    elf->header.phentsize = 0;
  }
}

// Write an ELF file to the given file pointer.
void ELFWriterFileWrite(ELFWriterFile* elf, FILE* fp) {
  // Add symbol table, section names and string table.
  ELFWriterSection* symtab = ELFWriterAddStandardSection(elf, ".symtab", SHT(symtab), 0);
  ELFWriterSection* strtab = ELFWriterAddStandardSection(elf, ".strtab", SHT(strtab), SHF(strings));
  ELFWriterSection* shstrtab = ELFWriterAddStandardSection(elf, ".shstrtab", SHT(strtab), SHF(strings));
  
  // Setup the special symbol table fields.
  symtab->header.entsize = elf->ops->symbol_size;
  ELFWriterAddSectionFixup(elf, kFixupFieldLink, symtab, strtab);

  // The 'info' field is one greater than the index of the last local
  // symbol.
  symtab->header.info = elf->last_local_symbol_index + 1;
  
  // Vector containing pointers to sections for relocations.  The
  // ELFWriterSection pointer is not owned by this vector.
  Vector relocation_sections = {0};
  
  // Create the relocation sections.
  CreateRelocationSections(elf, &relocation_sections,
                           symtab);

  elf->header.shnum = elf->sections.length;
  elf->header.shstrndx = shstrtab->index;

  // Fixup all the section links.
  ELFWriterFixupSections(elf);
  
  // We always add a PHDR segment as the first segment.  This covers the segment
  // headers and is part of the code segment.  Only do this if there are actually
  // segments in the file.
  size_t num_segments = elf->segments.length == 0 ? 0 : elf->segments.length + 1;

  elf->header.shoff = elf->ops->header_size +
      num_segments * elf->ops->program_header_size;

  // Allocate space for the file header.  This will be written after
  // we have all the information we need for it.
  // Also add space for the program segment headers. This will put the
  // file pointer at the start of the section headers.
  fseek(fp, elf->header.shoff, SEEK_SET);
  
  // Write all the section headers to the file.
  WriteSectionHeaders(elf, symtab, strtab,
                     shstrtab,
                     &relocation_sections,
                     num_segments, fp);

  if (elf->dynamic_callback != NULL) {
    // If we have a callback for a dynamic library, call it now.
    elf->dynamic_callback(elf);
  }
  
  // Write the section data to the file.
  WriteSectionContents(elf, symtab, strtab,
                       shstrtab,
                       &relocation_sections, fp);
  

  // Handle segments.  These have references to sections which,
  // by this point, will have been given addresses and offsets.
  WriteProgramHeaders(elf, num_segments, fp);
  
  // Now we need to go back and write the file header.
  rewind(fp);
  elf->ops->WriteHeader(&elf->header, fp);
  
  // Don't need the relocation sections vector now.
  VectorDestruct(&relocation_sections);
}


ELFWriterFile* NewELFWriterFileFromFile(FILE* fp) {
  ELFWriterFile* elf = malloc(sizeof(ELFWriterFile));
  // TODO
  return elf;
}


void ELFWriterSectionDestruct(ELFWriterSection* sect) {
  StringDestruct(&sect->name);
  if (sect->relocations != NULL) {
    VectorDelete(sect->relocations);
  }
}

void ELFWriterSectionDelete(ELFWriterSection* sect) {
  ELFWriterSectionDestruct(sect);
  free(sect);
}

ELFWriterSection* ELFWriterAddSection(ELFWriterFile* elf, String* name, int32_t type, int64_t flags,
                          int64_t alignment, ELFWriterSectionContents* contents, uint64_t address) {
  ELFWriterSection* section = calloc(sizeof(ELFWriterSection), 1);
  if (name != NULL) {
    StringInit(&section->name, name->value);
    section->header.name = ELFWriterAddSectionName(elf, name->value);
  } else {
    StringInit(&section->name, "");
  }
  section->header.type = type;
  section->header.flags = flags;
  section->header.addralign = alignment;
  section->header.addr = address;
  section->header.info = 0;
  section->contents = contents;
  section->relocations = NewVector();
  section->index = (int32_t)elf->sections.length;
  section->address = address;
  section->padding = 0;
  section->user_data = NULL;
  VectorAppend(&elf->sections, section);
  return section;
}

ELFWriterSection* ELFWriterAddStandardSection(ELFWriterFile* elf, const char* name,
                          ELFSectionType type,
                          ELFSectionFlags flags) {
  ELFWriterSection* section = calloc(sizeof(ELFWriterSection), 1);
  StringInit(&section->name, name);
  section->header.name = name == NULL ? 0 : ELFWriterAddSectionName(elf, name);
  section->header.type = type;
  section->header.flags = flags;
  section->header.addralign = 8;
  section->header.info = 0;
  section->contents = NULL;
  section->index = (int32_t)elf->sections.length;
  section->relocations = NewVector();
  section->user_data = NULL;
  VectorAppend(&elf->sections, section);
  return section;
}

void ELFSymbolInit(ELFSymbol* sym, ELF_Word name_offset,
                    int32_t section_index, int32_t symbol_type,
                    int32_t symbol_binding, int64_t size,
                    int64_t value) {
  sym->name = name_offset;
  sym->shndx = section_index;
  sym->other = 0;
  sym->size = size;
  sym->value = value;
  sym->info = symbol_type | symbol_binding << 4;
}
                   
ELFSymbol* NewELFSymbol(ELF_Word name_offset,
                            int32_t section_index, int32_t symbol_type,
                            int32_t symbol_binding, int64_t size,
                            int64_t value) {
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  ELFSymbolInit(sym, name_offset, section_index, symbol_type,
                symbol_binding, size, value);
  return sym;
}

ELFSymbol* ELFWriterAddSymbol(ELFWriterFile* elf,
                              String* name,
                              int32_t section_index,
                              int32_t symbol_type,
                              int32_t symbol_binding,
                              int64_t size,
                              int64_t value,
                              int32_t* index) {
  ELFSymbol* sym = NewELFSymbol(
                             ELFWriterAddString(elf, name),
                             section_index, symbol_type,
                             symbol_binding, size, value);
 
  // Set index in symbol now that we know it.
  *index = (int32_t)elf->symbol_table.length;
  VectorAppend(&elf->symbol_table, sym);
  
  return sym;
}

ELFSymbol* ELFWriterAddSectionSymbol(ELFWriterFile* elf, String* name, int32_t index) {
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  sym->name = ELFWriterAddString(elf, name);
  sym->info |= STT(section);
  sym->info |= STB(local);
  sym->shndx = index;
  VectorAppend(&elf->symbol_table, sym);
  return sym;
}


// Add a file symbol for the given filename.
ELFSymbol* ELFWriterAddFileSymbol(ELFWriterFile* elf, String* filename) {
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  sym->name = ELFWriterAddString(elf, filename);
  sym->shndx = SHN_ABS;
  sym->info |= STT(file);
  sym->info |= STB(local);
  
  VectorAppend(&elf->symbol_table, sym);
  return sym;
}

void ELFWriterInitRelocation(ELFRelocation* r,
                             int64_t offset,
                             int32_t symbol_index,
                             int64_t addend,
                             int32_t type) {
  r->offset = offset;      // Offset into section.
  r->info = (ELF_Xword)symbol_index << 32 | type;
  r->addend = addend;
  
}

void ELFWriterAddRelocationWithAddend(ELFWriterFile* elf,
                                      int32_t section_index,
                                      int64_t offset,
                                      int32_t symbol_index,
                                      int64_t addend,
                                      int32_t type) {
  ELFRelocation* r = malloc(sizeof(ELFRelocation));
  ELFWriterInitRelocation(r, offset, symbol_index, addend, type);
  assert(symbol_index != -1);
  ELFWriterInsertRelocation(elf, section_index, r);
}

void ELFWriterAddRelocation(ELFWriterFile* elf,
                            int32_t section_index,
                            int64_t offset,
                            int32_t symbol_index,
                            int32_t type) {
  ELFWriterAddRelocationWithAddend(elf, section_index, offset,
                                   symbol_index, 0, type);
}

void ELFWriterInsertRelocation(ELFWriterFile* elf,
                                      int32_t section_index,
                                      ELFRelocation* reloc) {
  ELFWriterSection* section = elf->sections.value.p[section_index];
  VectorAppend(section->relocations, reloc);
}


ELF_Word ELFWriterAddString(ELFWriterFile* elf, String* str) {
  return AddBufferedString(&elf->string_table, str);
}



ELF_Word ELFWriterAddRawString(ELFWriterFile* elf, const char* str) {
  ELF_Word offset = (ELF_Word)elf->string_table.length;
  size_t len = strlen(str) + 1;
  BufferAppend(&elf->string_table, (char*)str, len);
  return offset;
}

ELF_Word ELFWriterAddSectionName(ELFWriterFile* elf, const char* name) {
  ELF_Word offset = (ELF_Word)elf->section_names.length;
  size_t len = strlen(name) + 1;
  BufferAppend(&elf->section_names, (char*)name, len);
  return offset;
}

ELFWriterSegment* NewELFWriterSegment(int32_t type, int32_t flags, int64_t alignment) {
  ELFWriterSegment* segment = malloc(sizeof(ELFWriterSegment));
  ELFProgramHeader* hdr = &segment->header;
  hdr->type = type;
  hdr->align = alignment;
  hdr->filesz = 0;
  hdr->memsz = 0;
  hdr->offset = 0;
  hdr->paddr = hdr->vaddr = 0;
  hdr->flags = flags;
  VectorInit(&segment->sections);
  return segment;
}

void ELFWriterSegmentDestruct(ELFWriterSegment* segment) {
  VectorDestruct(&segment->sections);
}

void ELFWriterSegmentDelete(ELFWriterSegment* segment) {
  ELFWriterSegmentDestruct(segment);
  free(segment);
}

void ELFWriterSegmentAddSection(ELFWriterSegment* segment, ELFWriterSection* section) {
  VectorAppend(&segment->sections, section);
}

