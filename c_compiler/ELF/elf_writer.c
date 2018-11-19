//
//  elf_writer.c
//  c_compiler
//
//  Created by David Allison on 1/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "elf_writer.h"
#include "stdlib.h"
#include <assert.h>
#include <string.h>

void ELFWriterFileInit(ELFWriterFile* elf, ELFType type, int machine, int flags, bool dso) {
  memset(&elf->header, 0, sizeof(elf->header));
  elf->header.ident[EI_MAG0] = '\x7f';
  elf->header.ident[EI_MAG1] = 'E';
  elf->header.ident[EI_MAG2] = 'L';
  elf->header.ident[EI_MAG3] = 'F';
  elf->header.ident[EI_CLASS] = 2;      // 64 bit.
  elf->header.ident[EI_DATA] = 1;       // LSB.
  elf->header.ident[EI_VERSION] = 1;
  elf->header.ident[EI_OSABI] = 0;
  elf->header.type = type;
  elf->header.machine = machine;
  elf->header.flags = flags;
  elf->header.version = 1;
  elf->header.ehsize = sizeof(ELFHeader);
  elf->header.shentsize = sizeof(ELFSectionHeader);
  elf->header.phentsize = sizeof(ELFProgramHeader);
  elf->header.phoff = sizeof(ELFHeader);      // Program segment headers after file header.
  VectorInit(&elf->sections);
  VectorInit(&elf->segments);
  BufferInit(&elf->string_table);
  VectorInit(&elf->symbol_table);
  BufferInit(&elf->dyn_string_table);
  VectorInit(&elf->dyn_symbol_table);
  BufferInit(&elf->section_names);
  
  // Add first (empty) symbol to the symbol table.  All fields
  // of this symbol are zero.
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  VectorAppend(&elf->symbol_table, sym);
  
  // The first valid string table index is 1, so add a zero
  // to the start of it.
  BufferAppend(&elf->string_table, "", 1);
  BufferAppend(&elf->section_names, "", 1);
  elf->dso = dso;
}

void ELFWriterFileDestruct(ELFWriterFile* elf) {
  // TODO: delete the objects in the vectors.
  VectorDestruct(&elf->sections);
  VectorDestruct(&elf->segments);
  BufferDestruct(&elf->string_table);
  VectorDestruct(&elf->symbol_table);
  BufferDestruct(&elf->dyn_string_table);
  VectorDestruct(&elf->dyn_symbol_table);
  BufferDestruct(&elf->section_names);
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
      break;
  }
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
        size += ELFWriterSectionContentsGetLength(contents->data.multi.value[i]);
      }
      return size;
    }
    case kSectionContentsNobits:
      return contents->size;
  }
}

void ELFWriterSectionContentsWrite(ELFWriterSectionContents* contents, FILE* fp) {
  switch (contents->data_location) {
    case kSectionContentsBuffered:
      fwrite(contents->data.buffered.value, contents->data.buffered.length, 1, fp);
      break;
    case kSectionContentsRaw:
      fwrite(contents->data.raw, contents->size, 1, fp);
      break;
    case kSectionContentsMulti: {
      for (size_t i = 0; i < contents->data.multi.length; i++) {
        ELFWriterSectionContentsWrite(contents->data.multi.value[i], fp);
      }
      break;
    }
    case kSectionContentsNobits:
      break;
  }
}

void ELFWriterFileWrite(ELFWriterFile* elf, FILE* fp) {
  // Add symbol table, section names and string table.
  ELFWriterSection* symtab = ELFWriterAddNamedSection(elf, ".symtab", SHT(symtab), 0);
  ELFWriterSection* strtab = ELFWriterAddNamedSection(elf, ".strtab", SHT(strtab), 0);
  ELFWriterSection* shstrtab = ELFWriterAddNamedSection(elf, ".shstrtab", SHT(strtab), 0);
  
  // Setup the special symbol table fields.
  symtab->header.entsize = sizeof(ELFSymbol);
  
  // The 'link' field is the index of the symbol names string table.
  symtab->header.link = strtab->index;
  
  // The 'info' field is one greater than the index of the last local
  // symbol.
  symtab->header.info = elf->last_local_symbol_index + 1;
  
  // If we are writing a DSO we need to allocate the .dynsym and .dynstr
  // sections for the dynamic symbol table.  These are separate from the
  // regular symbol and string tables to allow the files to be stripped and
  // still be loadable.
  ELFWriterSection* dyn_symtab = NULL;
  ELFWriterSection* dyn_strtab = NULL;
  if (elf->dso) {
    dyn_symtab = ELFWriterAddNamedSection(elf, ".dynsym", SHT(symtab), 0);
    dyn_strtab = ELFWriterAddNamedSection(elf, ".dynstr", SHT(strtab), 0);
    dyn_symtab->header.entsize = sizeof(ELFSymbol);
    dyn_symtab->header.link = dyn_strtab->index;
    dyn_symtab->header.info = 1;      // TODO: is this necessary?
  }
  
  Vector relocation_sections;
  VectorInit(&relocation_sections);
  
  // Add relocation sections.
  // Record current section length because we will be adding sections
  // to the sections vector in the loop.
  size_t num_sections = elf->sections.length;
  for (size_t i = 0; i < num_sections; i++) {
    ELFWriterSection* section = elf->sections.value[i];
    if (section->relocations->length != 0) {
      String relocation_section_name;
      StringInit(&relocation_section_name, "");
      StringPrintf(&relocation_section_name, ".rela%s", section->name.value);
      ELFWriterSection* reloc_sect = ELFWriterAddNamedSection(elf, relocation_section_name.value,
                                            SHT(rela), 0);
      // All relocation sections link to the symtab section so that
      // they can find the symbol.
      reloc_sect->header.link = symtab->index;
      
      reloc_sect->header.info = section->index;
      
      // RELA sections have a fixed entry size.
      reloc_sect->header.entsize = sizeof(ELFRelocation);
      // Take ownership of the relocations.
      reloc_sect->relocations = section->relocations;
      section->relocations = NULL;
      
      // Keep track of the relocation section.  It now owns
      // all the relocations so we know what its size and
      // contents are.
      VectorAppend(&relocation_sections, reloc_sect);
      StringDestruct(&relocation_section_name);
    }
  }
  elf->header.shnum = elf->sections.length;
  elf->header.shstrndx = shstrtab->index;

  // We always add a PHDR segment as the first segment.  This covers the segment
  // headers and is part of the code segment.  Only do this if there are actually
  // segments in the file.
  size_t num_segments = elf->segments.length == 0 ? 0 : elf->segments.length + 1;

  elf->header.shoff = sizeof(ELFHeader) + num_segments * sizeof(ELFProgramHeader);

  // Allocate space for the file header.  This will be written after
  // we have all the information we need for it.
  // Also add space for the program segment headers. This will put the file pointer
  // at the start of the section headers.
  fseek(fp, elf->header.shoff, SEEK_SET);

  // Calculate the file offset for the first section data.   This is incremented
  // during the offset assignment loop below to be the address of the next
  // setion's file offset.
  int64_t next_section_data_offset = sizeof(ELFHeader) +
    num_segments * sizeof(ELFProgramHeader) +
    elf->sections.length * sizeof(ELFSectionHeader);
  
  // Write all the section headers to the file.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFWriterSection* section = elf->sections.value[i];
    size_t data_length = 0;
    if (section->contents != NULL) {
      // Get section data length.
      data_length = ELFWriterSectionContentsGetLength(section->contents);
    } else {
      if (section == symtab) {
        data_length = elf->symbol_table.length * sizeof(ELFSymbol);
      } else if (section == dyn_symtab) {
        data_length = elf->dyn_symbol_table.length * sizeof(ELFSymbol);
      } else if (section == strtab) {
        data_length = elf->string_table.length;
      } else if (section == dyn_strtab) {
        data_length = elf->dyn_string_table.length;
      } else if (section == shstrtab) {
        data_length = elf->section_names.length;
      } else {
        bool section_found = false;
        for (size_t j = 0; j < relocation_sections.length; j++) {
          if (relocation_sections.value[j] == section) {
            data_length = section->relocations->length * sizeof(ELFRelocation);
            section_found = true;
            break;
          }
        }
        assert(section_found);
      }
    }
    if (section->header.name != 0) {
      section->header.offset = next_section_data_offset;
    }
    section->header.size = data_length;

    fwrite(&section->header, sizeof(ELFSectionHeader), 1, fp);
    if (section->header.type != SHT(nobits)) {
      next_section_data_offset += data_length;
    }
  }
  
  // Write the section data to the file.
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFWriterSection* section = elf->sections.value[i];
    size_t data_length = 0;
    if (section->contents != NULL) {
      ELFWriterSectionContentsWrite(section->contents, fp);
    } else {
      if (section == symtab) {
        for (size_t sym_index = 0; sym_index < elf->symbol_table.length; sym_index++) {
          ELFSymbol* sym = elf->symbol_table.value[sym_index];
          fwrite(sym, sizeof(ELFSymbol), 1, fp);
        }
      } else if (section == dyn_symtab) {
          for (size_t sym_index = 0; sym_index < elf->dyn_symbol_table.length; sym_index++) {
            ELFSymbol* sym = elf->dyn_symbol_table.value[sym_index];
            fwrite(sym, sizeof(ELFSymbol), 1, fp);
          }
      } else if (section == strtab) {
        data_length = elf->string_table.length;
        fwrite(elf->string_table.value, data_length, 1, fp);
      } else if (section == dyn_strtab) {
        data_length = elf->dyn_string_table.length;
        fwrite(elf->dyn_string_table.value, data_length, 1, fp);
      } else if (section == shstrtab) {
        data_length = elf->section_names.length;
        fwrite(elf->section_names.value, data_length, 1, fp);
      } else {
        bool section_found = false;
        // Write out the relocation section contents.
        for (size_t j = 0; j < relocation_sections.length; j++) {
          if (relocation_sections.value[j] == section) {
            for (size_t reloc_index = 0;
                 reloc_index < section->relocations->length; reloc_index++) {
              ELFRelocation* reloc = section->relocations->value[reloc_index];
              fwrite(reloc, sizeof(*reloc), 1, fp);
            }
            section_found = true;
            break;
          }
        }
        assert(section_found);
      }
    }
  }

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
    phdr.filesz = num_segments * sizeof(ELFProgramHeader);
    phdr.memsz = phdr.filesz;
    phdr.align = 8;
    phdr.flags = PF(r) | PF(x);
    phdr.type = PT(phdr);

    ELFWriterSegment* code_segment = elf->segments.value[0];
    ELFWriterSection* section0 = code_segment->sections.value[0];
    phdr.vaddr = section0->address + elf->header.phoff;
    phdr.paddr = phdr.vaddr;
    fwrite(&phdr, sizeof(phdr), 1, fp);

    // The first segment is at offset 0.  The next one starts at the offset
    // of the first section held within it.
    bool segment_includes_file_header = true;
    int64_t first_section_offset = 0;

    for (size_t i = 0; i < elf->segments.length; i++) {
      ELFWriterSegment* segment = elf->segments.value[i];
      bool start_address_assigned = false;
      for (size_t j = 0; j < segment->sections.length; j++) {
        ELFWriterSection* section = segment->sections.value[j];

        // Add section size to the memory size.
        segment->header.memsz += section->header.size;

        // If the section is present in the file (not NOBITS), add its
        // size to the filesz.
        if (section->header.type != SHT(nobits)) {
          segment->header.filesz += section->header.size;
        }

        // The first section gives us the offset and addresses for the segment.
        if (!start_address_assigned) {
          segment->header.offset = i == 0 ? 0 : section->header.offset;
          segment->header.vaddr = section->address;
          segment->header.paddr = segment->header.vaddr;
          start_address_assigned = true;
          first_section_offset = section->header.offset;
        }
      }

      // The first segment includes the ELF file header.  So we need to adjust its file
      // and memory sizes to account of for it.
      if (segment_includes_file_header) {
        segment->header.filesz += first_section_offset;
        segment->header.memsz += first_section_offset;
        segment_includes_file_header = false;
      }
      fwrite(&segment->header, sizeof(ELFProgramHeader), 1, fp);
    }
  } else {
    // No segments, need to zero out the phoff
    elf->header.phoff = 0;
    elf->header.phentsize = 0;
  }

  // Now we need to go back and write the file header.

  rewind(fp);
  fwrite(&elf->header, sizeof(ELFHeader), 1, fp);
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
  section->contents = contents;
  section->relocations = NewVector();
  section->index = (int32_t)elf->sections.length;
  section->address = address;
  VectorAppend(&elf->sections, section);
  return section;
}

ELFWriterSection* ELFWriterAddNamedSection(ELFWriterFile* elf, const char* name,
                          ELFSectionType type,
                          ELFSectionFlags flags) {
  ELFWriterSection* section = calloc(sizeof(ELFWriterSection), 1);
  StringInit(&section->name, name);
  section->header.name = name == NULL ? 0 : ELFWriterAddSectionName(elf, name);
  section->header.type = type;
  section->header.flags = flags;
  section->header.addralign = 1;
  section->contents = NULL;
  section->index = (int32_t)elf->sections.length;
  section->relocations = NewVector();
  VectorAppend(&elf->sections, section);
  return section;
}

static ELFSymbol* NewSymbol(ELFWriterFile* elf, ELF_Word name_offset,
                            int32_t section_index, int32_t symbol_type,
                            int32_t symbol_binding, int64_t size,
                            int64_t value) {
  ELFSymbol* sym = calloc(sizeof(ELFSymbol), 1);
  sym->name = name_offset;
  sym->shndx = section_index;
  
  sym->size = size;
  sym->value = value;
  sym->info = symbol_type | symbol_binding << 4;
  return sym;
}

ELFSymbol* ELFWriterAddSymbol(ELFWriterFile* elf, String* name, int32_t section_index, int32_t symbol_type,
                        int32_t symbol_binding, int64_t size, int64_t value, int32_t* index) {
  ELFSymbol* sym = NewSymbol(elf, ELFWriterAddString(elf, name),
                             section_index, symbol_type, symbol_binding, size, value);
 
  // Set index in symbol now that we know it.
  *index = (int32_t)elf->symbol_table.length;
  VectorAppend(&elf->symbol_table, sym);
  
  if (elf->dso && symbol_binding == STB(global)) {
    // If we are building a DSO, add the symbol to the dynamic symbol table,
    // as long as the symbol is globa (or undefined).
    ELFSymbol* sym = NewSymbol(elf, ELFWriterAddDynamicString(elf, name),
                               section_index, symbol_type,
                               symbol_binding, size, value);

    VectorAppend(&elf->dyn_symbol_table, sym);
  }
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

void ELFWriterAddRelocationWithAddend(ELFWriterFile* elf, int32_t section_index, int64_t offset, int32_t symbol_index, int64_t addend, int32_t type) {
  ELFWriterSection* section = elf->sections.value[section_index];
  ELFRelocation* r = malloc(sizeof(ELFRelocation));

  r->offset = offset;      // Offset into section.

  assert(symbol_index != -1);
  r->info = (ELF_Xword)symbol_index << 32 | type;
  r->addend = addend;
  VectorAppend(section->relocations, r);
}

void ELFWriterAddRelocation(ELFWriterFile* elf, int32_t section_index, int64_t offset, int32_t symbol_index, int32_t type) {
  ELFWriterAddRelocationWithAddend(elf, section_index, offset, symbol_index, 0, type);
}

ELF_Word ELFWriterAddString(ELFWriterFile* elf, String* str) {
  ELF_Word offset = (ELF_Word)elf->string_table.length;
  BufferAppend(&elf->string_table, str->value, str->length + 1);
  return offset;
}

ELF_Word ELFWriterAddDynamicString(ELFWriterFile* elf, String* str) {
  ELF_Word offset = (ELF_Word)elf->dyn_string_table.length;
  BufferAppend(&elf->dyn_string_table, str->value, str->length + 1);
  return offset;
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

void ELFWriterSegmentDelete(ELFWriterSegment* segment) {
  VectorDestruct(&segment->sections);
  free(segment);
}

void ELFWriterSegmentAddSection(ELFWriterSegment* segment, ELFWriterSection* section) {
  VectorAppend(&segment->sections, section);
}

