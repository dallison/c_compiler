//
//  linker_dynamic.c
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker.h"
#include "linker_arch.h"
#include "linker_dynamic.h"
#include <stdlib.h>
#include "linker_file.h"
#include "linker_symbols.h"
#include "linker_reloc.h"
#include "linker.h"
#include "elf_writer.h"
#include "p_code_machine.h"
#include "risc_v_machine.h"
#include <assert.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void DynamicLinkerInit(DynamicLinker* s, Linker* linker) {
  assert(linker->arch != NULL);

  // Initialize architecture specific info.
  linker->arch->init_dynamic_linker(s);

  VectorInit(&s->global_offset_table.data_entries);
  VectorInit(&s->global_offset_table.tls_ie_entries);
  VectorInit(&s->global_offset_table.tls_gd_entries);
  VectorInit(&s->global_offset_table.function_entries);
  VectorInit(&s->procedure_linkage_table.trampolines);
  VectorInit(&s->got_relocations);
  VectorInit(&s->plt_relocations);
  VectorInit(&s->data_relocations);
  VectorInit(&s->needed_libraries);
  s->rpath = 0;
  s->plt_group = NULL;
  s->got_group = NULL;
  s->got_plt_group = NULL;

  DynamicLibraryRegistryInit(&s->loaded_dynamic_libraries);
}

DynamicLinker* NewDynamicLinker(Linker* linker) {
  DynamicLinker* s = malloc(sizeof(DynamicLinker));
  DynamicLinkerInit(s, linker);
  return s;
}

void DynamicLinkerDestruct(DynamicLinker* s) {
  // TODO: vector contents.
  VectorDestruct(&s->global_offset_table.data_entries);
  VectorDestruct(&s->global_offset_table.tls_ie_entries);
  VectorDestruct(&s->global_offset_table.tls_gd_entries);
  VectorDestruct(&s->global_offset_table.function_entries);
  VectorDestruct(&s->procedure_linkage_table.trampolines);
  VectorDestruct(&s->got_relocations);
  VectorDestruct(&s->plt_relocations);
  VectorDestruct(&s->data_relocations);
  VectorInit(&s->needed_libraries);
  DynamicLibraryRegistryDestruct(&s->loaded_dynamic_libraries);
}

void DynamicLinkerDelete(DynamicLinker* s) {
  DynamicLinkerDestruct(s);
  free(s);
}

void DynamicLinkerInventSymbols(Linker* linker, DynamicLinker* s) {
  s->global_offset_table_symbol = LinkerInventSymbol(linker, "_GLOBAL_OFFSET_TABLE_", 8);
  s->dynamic_symbol = LinkerInventSymbol(linker, "_DYNAMIC_", 8);
}

void DynamicLinkerDefineSymbols(Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  
  // _GLOBAL_OFFSET_TABLE_
  dynamic->global_offset_table_symbol->address = dynamic->got_plt_group->address;
  dynamic->global_offset_table_symbol->defined = true;
  dynamic->global_offset_table_symbol->invented = true;

  // _DYNAMIC_
  dynamic->dynamic_symbol->address = dynamic->dynamic_group->address;
  dynamic->dynamic_symbol->defined = true;
  dynamic->dynamic_symbol->invented = true;
}



//
// DSO building functions.
//

static int GetDataGOTOffset(DynamicLinker* s, LinkerSymbol* symbol) {
  if (symbol->got_index == -1) {
    // LinkerSymbol is not in Global Offset Table, add it.
    VectorAppend(&s->global_offset_table.data_entries, symbol);
    symbol->got_index = (int)s->global_offset_table.data_entries.length - 1;
  }
  return symbol->got_index;
}

// NOTE: the function's GOT offset is relative to the end of
// the data GOT entries.  We don't know how many data entries
// there are yet, so the actual offset will need be calculated later.
static int GetFunctionGOTOffset(DynamicLinker* s, LinkerSymbol* symbol) {
  if (symbol->got_index == -1) {
    // LinkerSymbol is not in Global Offset Table, add it.
    VectorAppend(&s->global_offset_table.function_entries, symbol);
    symbol->got_index = (int)s->global_offset_table.function_entries.length - 1 +
      s->global_offset_table.num_resolver_data_entries;
  }
  return symbol->got_index;
}

static int GetPLTOffset(DynamicLinker* s, LinkerSymbol* symbol) {
  if (symbol->plt_index == -1) {
    VectorAppend(&s->procedure_linkage_table.trampolines, symbol);
    symbol->plt_index = (int)s->procedure_linkage_table.trampolines.length - 1 +
      s->procedure_linkage_table.num_reserved_entries;
  }
  return symbol->plt_index;
}



static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
                        GOTRelocation relocation_type) {
  linker->arch->add_got_entry(linker, symbol, contents, relocs, relocation_type);
}

// Build the .got section contents.
static void BuildGlobalOffsetTableContents(struct Linker* linker,
                            ELFWriterSectionContents* got_contents,
                            ELFWriterSectionContents* got_plt_contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  
  // Add data entries.
  Vector* data_entries = &dynamic->global_offset_table.data_entries;
  for (size_t i = 0; i < data_entries->length; i++) {
    AddGOTEntry(linker, data_entries->value.p[i], got_contents,
                &dynamic->got_relocations, kGOTRelocationVariable);
  }

  // TLS Initial Exec entries.
  Vector* tls_ie_entries = &dynamic->global_offset_table.tls_ie_entries;
  for (size_t i = 0; i < tls_ie_entries->length; i++) {
    AddGOTEntry(linker, tls_ie_entries->value.p[i], got_contents,
                &dynamic->got_relocations, kGOTRelocationTLSOffset);
  }

  // TLS Global Dynamic entries.
  Vector* tls_gd_entries = &dynamic->global_offset_table.tls_gd_entries;
  for (size_t i = 0; i < tls_gd_entries->length; i++) {
    // Global Dynamic entries have 2 slots in the GOT.  The first is the
    // module id and the second is the TLS offset.
    AddGOTEntry(linker, tls_gd_entries->value.p[i], got_contents,
                &dynamic->got_relocations, kGOTRelocationTLSModuleId);
    AddGOTEntry(linker, tls_gd_entries->value.p[i], got_contents,
                &dynamic->got_relocations, kGOTRelocationTLSOffset);
  }

  // Add space for resolver data entries.
  BufferAddSpace(&got_plt_contents->data.buffered,
                 dynamic->global_offset_table.num_resolver_data_entries *
                 dynamic->global_offset_table.entry_size);

  // Add function entries.
  
  Vector* func_entries = &dynamic->global_offset_table.function_entries;
  for (size_t i = 0; i < func_entries->length; i++) {
    LinkerSymbol* symbol = func_entries->value.p[i];
    AddGOTEntry(linker, symbol, got_plt_contents,
                &dynamic->plt_relocations, kGOTRelocationFunction);
  }
}

// Given a SectionGroup containing a single output section, get the
// Buffer corresponding to the data contained in that section.  The
// asserts make sure this is the correct format.
static Buffer* GetSectionContentsBuffer(SectionGroup* group) {
  assert(group->components.length == 1);
  GroupedSection* sect = group->components.value.p[0];
  assert(sect->source == kGroupedSectionNew);
  assert(sect->section.new->contents->data_location == kSectionContentsBuffered);
  ELFWriterSectionContents* contents = sect->section.new->contents;
  assert(contents->data_location == kSectionContentsBuffered);
  return &contents->data.buffered;
}

// The GOT entries for the PLT (function entries) are set
// to contain an address inside the PLT trampoline for the
// given symbol.
//
// The address inside the trampoline depends on the architecture.
// For P-Code, the address is 12 bytes after the trampoline start - the
// address of the movxc instruction for the dynamic symbol resolver.
void DynamicLinkerFixupGOT(Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  Buffer* got_plt_buffer = GetSectionContentsBuffer(dynamic->got_plt_group);
  uint64_t plt_address = dynamic->plt_group->address;
  
  Vector* func_entries = &dynamic->global_offset_table.function_entries;
  for (size_t i = 0; i < func_entries->length; i++) {
    LinkerSymbol* symbol = func_entries->value.p[i];
    // The got_index inside the symbol is the absolute index
    // into the GOT.  The plt_index is the absolute index
    // into the PLT.
    //
    linker->arch->fixup_got_entry(symbol, got_plt_buffer, plt_address,
                                  dynamic->procedure_linkage_table.entry_size);
  }
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  linker->arch->add_plt_entry(linker, symbol, contents);
}

// Buid the architecture-specific symbol resolution entry at the
// start of the PLT.
static void BuildPLTResolveSymbolEntry(Linker* linker,
                                       ELFWriterSectionContents* contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  
  BufferAddSpace(&contents->data.buffered,
                 dynamic->procedure_linkage_table.num_reserved_entries *
                 dynamic->procedure_linkage_table.entry_size);
}

static void BuildProcedureLinkageTableContents(Linker* linker,
                                               ELFWriterSectionContents* contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  
  // Table begins with symbol resolution code.
  BuildPLTResolveSymbolEntry(linker, contents);
  
  // Add plt entries, one entry per called procedure.
  for (size_t i = 0;
       i < dynamic->procedure_linkage_table.trampolines.length;
       i++) {
    AddPLTEntry(linker,
                dynamic->procedure_linkage_table.trampolines.value.p[i],
                contents);
  }
}

// The PLT needs to contain the address of the GOT entry for the symbol
// to which the trampoline jumps.  This is dependent on the architecture.
void DynamicLinkerFixupPLT(Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  ProcedureLinkageTable* plt = &dynamic->procedure_linkage_table;
  GlobalOffsetTable* got = &dynamic->global_offset_table;
  Buffer* plt_buffer = GetSectionContentsBuffer(dynamic->plt_group);
  uint64_t got_address = dynamic->got_plt_group->address;
  uint64_t plt_address = dynamic->plt_group->address;
  
  // Add the first entry to the PLT.  This is a call to the dynamic loader's
  // symbol resolver.
  linker->arch->setup_resolver_plt_entry(plt, plt_buffer, got_address, plt_address);
  
  // Fixup the actual PLT trampolines now.
  for (size_t i = 0; i < plt->trampolines.length; i++) {
    LinkerSymbol* sym = plt->trampolines.value.p[i];
    linker->arch->fixup_plt_entry(plt, got,
                                  sym, plt_buffer,
                                  got_address, plt_address);
  }
}

// Look at a relocation and see if it's a PIC relocation,
// that references a GOT or PLT entry.  Use the information
// to build the GOT and PLT.  The symbol is given two
// fields:
// got_index: index into the GOT
// plt_index: index into the PLT.
static void ProcessPossibleDynamicRelocation(struct Linker* linker,
                                             struct ObjectFile* file,
                                             Relocation* reloc) {
  LinkerSymbol* symbol = ObjectFileFindSymbol(file, reloc->symbol_name.value);
  linker->arch->handle_pic_relocation(linker->dynamic_linker,
                                        symbol,
                                        reloc,
                                        GetDataGOTOffset,
                                        GetFunctionGOTOffset,
                                        GetPLTOffset);
}

void DynamicLinkerGatherDynamicRelocations(Linker* linker) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    ObjectFile* file = linker->files.value.p[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      ProcessPossibleDynamicRelocation(linker, file, file->relocations.value.p[reloc_index]);
    }
  }
}

// We don't have all the information we need for the dynamic relocations
// yet but we need to allocate the space for it inside the text segment.
// This allocates the buffer space.
static void AllocateDynamicRelocations(struct Linker* linker,
                             struct ELFWriterSectionContents* contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  
  // The .rela.dyn section contains both GOT and data reloacations.
  size_t length = (dynamic->got_relocations.length +
                   dynamic->data_relocations.length) * sizeof(ELFRelocation);
  BufferAddSpace(&contents->data.buffered, length);
}

// We don't have all the information we need for the plt relocations
// yet but we need to allocate the space for it inside the text segment.
// This allocates the buffer space.
static void AllocatePLTRelocations(struct Linker* linker,
                                       struct ELFWriterSectionContents* contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  BufferAddSpace(&contents->data.buffered,
                 dynamic->plt_relocations.length * sizeof(ELFRelocation));
}


// Build the dynamic relocation section contents now that we know all
// the information for the relocations.
// The relocations have an offset from the start of the library, not
// the start of the .got section.
void DynamicLinkerBuildDynamicRelocations(struct Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  GroupedSection* gsect = dynamic->dyn_rela_group->components.value.p[0];
  Buffer* contents = &gsect->section.new->contents->data.buffered;
  
  // The buffer already has all the space we need but it is not populated.
  // Since we now know the symbol indexes we can create the relocation
  // entries in the table, overwriting the memory previously allocated.
  
  // Data relocations come first.
  int32_t index = 0;
  for (size_t i = 0; i < dynamic->data_relocations.length; i++) {
    Relocation* reloc = dynamic->data_relocations.value.p[i];
    ELFRelocation* elfreloc = (ELFRelocation*)&contents->value[index*sizeof(ELFRelocation)];
    int64_t offset = reloc->offset + reloc->section->address;
    ELFWriterInitRelocation(elfreloc, offset,
                            0, 0, reloc->type);
    index++;
  }

  // Now GOT relocations.
  for (size_t i = 0; i < dynamic->got_relocations.length; i++) {
    Relocation* reloc = dynamic->got_relocations.value.p[i];
    ELFRelocation* elfreloc = (ELFRelocation*)&contents->value[index*sizeof(ELFRelocation)];
    int symbol_index;
    if (reloc->symbol->dynamic_index == -1) {
      symbol_index = reloc->symbol->index;
    } else {
      symbol_index = reloc->symbol->dynamic_index;
    }
    assert(symbol_index != -1);
    ELFWriterInitRelocation(elfreloc, reloc->offset + dynamic->got_group->address,
                           symbol_index, 0, reloc->type);
    index++;
  }
}

// The relocations into the PLT have an offset from the start address
// of the library, not from the start of the .got.plt section.
void DynamicLinkerBuildPLTRelocations(struct Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  GroupedSection* gsect = dynamic->plt_rela_group->components.value.p[0];
  Buffer* contents = &gsect->section.new->contents->data.buffered;
  
  // The buffer already has all the space we need but it is not populated.
  // Since we now know the symbol indexes we can create the relocation
  // entries in the table, overwriting the memory previously allocated.
  for (size_t i = 0; i < dynamic->plt_relocations.length; i++) {
    Relocation* reloc = dynamic->plt_relocations.value.p[i];
    ELFRelocation* elfreloc = (ELFRelocation*)&contents->value[i*sizeof(ELFRelocation)];
    int symbol_index;
    if (reloc->symbol->dynamic_index == -1) {
      symbol_index = reloc->symbol->index;
    } else {
      symbol_index = reloc->symbol->dynamic_index;
    }
    // assert(symbol_index != -1);
    ELFWriterInitRelocation(elfreloc,
                            reloc->offset + dynamic->got_plt_group->address,
                            symbol_index, 0, reloc->type);
  }
}


// Build a new ELFWriterSection with a given name for later addition to the
// ELF file.
// This is not suitable for writing directly to an ELF file.  In particular the
// following things still need to be done.
// 1.  The name field needs to be added to a string table.
// 2.  There is no address assigned.
// 3.  There is no section index assigned.
static ELFWriterSection* NewELFSection(const char* name, int32_t type,
                                       int64_t flags,
                                       int64_t alignment,
                                       ELFWriterSectionContents* contents) {
  ELFWriterSection* section = calloc(sizeof(ELFWriterSection), 1);
  StringInit(&section->name, name);
  section->header.type = type;
  section->header.flags = flags;
  section->header.addralign = alignment;
  section->header.addr = 0;
  section->contents = contents;
  section->relocations = NewVector();
  section->index = 0;
  section->address = 0;
  section->user_data = NULL;
  return section;
}

// Given a section, build a SectionGroup containing it and add
// it to a given segment.  Return the group.
static SectionGroup* NewDynamicLinkerGroup(Linker* linker,
                                            ELFWriterSection* section,
                                            Segment* segment) {
  SectionGroup* group = NewSectionGroup(&section->name,
                                        section->header.type,
                                        section->header.flags,
                                        section->header.addralign);
  VectorAppend(&group->components, NewGroupedSection(section));
  group->segment = segment;
  VectorAppend(&segment->sections, group);
  VectorAppend(&linker->section_groups, group);
  SegmentMemoryRegion* region = SegmentDefaultRegion(segment);
  VectorAppend(&region->sections, NewString(section->name.value));
  return group;
}

// Add the Global Offset Table (.got) section and group.  Also
// add a .got.plt section that contains the GOT entries for functions.
// The .got section contains the addresses of symbols used as variables
// and have a relocation to set the absolute address at load time.
//
// The .got.plt section starts with some reserved runtime resolver
// data and then contains entries for functions.  These are initially
// set to refer to the PLT entry for that function and will be relocated
// at runtime (on first call) to point to the absolute address of the
// function.  The .dynamic section's DT(pltgot) will contain the address
// of the start of the .got.plt section so that the loader can find
// it easily.
static void AddGlobalOffsetTable(Linker* linker, SectionGroup** got_group,
                                          SectionGroup** got_plt_group) {
   ELFWriterSectionContents* got_contents =
      NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* got_section = NewELFSection(".got",
                                                SHT(progbits),
                                                SHF(write) | SHF(alloc),
                                                8, got_contents);
  
  
  ELFWriterSectionContents* got_plt_contents =
    NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* got_plt_section = NewELFSection(".got.plt",
                                            SHT(progbits),
                                            SHF(write) | SHF(alloc),
                                            8, got_plt_contents);

  got_section->header.entsize =
    linker->dynamic_linker->global_offset_table.entry_size;
  got_plt_section->header.entsize =
    linker->dynamic_linker->global_offset_table.entry_size;
  
  BuildGlobalOffsetTableContents(linker, got_contents, got_plt_contents);
 
  // Align length.
  BufferAlignLength(&got_contents->data.buffered, 8);
  BufferAlignLength(&got_plt_contents->data.buffered, 8);
  
  *got_group = NewDynamicLinkerGroup(linker,
                               got_section,
                               &linker->data_segment);

  *got_plt_group = NewDynamicLinkerGroup(linker,
                                got_plt_section,
                                &linker->data_segment);
}

// Add the Procedure Linkage Table (.plt) offset and group.
static SectionGroup* AddProcedureLinkageTable(Linker* linker) {
  ELFWriterSectionContents* contents =
    NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* section = NewELFSection(".plt",
                                            SHT(progbits),
                                            SHF(alloc),
                                            8, contents);
  section->header.entsize =
  linker->dynamic_linker->procedure_linkage_table.entry_size;
  
  BuildProcedureLinkageTableContents(linker, contents);
 
  // Align length.
  BufferAlignLength(&contents->data.buffered, 8);

  return NewDynamicLinkerGroup(linker,
                                section,
                                &linker->code_segment);
}

// Add a section for the relocations for the .got section.
static SectionGroup* AddDynamicRelocationsSection(Linker* linker) {
  ELFWriterSectionContents* contents =
  NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* section = NewELFSection(".rela.dyn",
                                            SHT(rela),
                                            SHF(alloc),
                                            8, contents);
  section->header.entsize = sizeof(ELFRelocation);
  
  // Allocate space for the relocations but we don't know the
  // contents yet.
  AllocateDynamicRelocations(linker, contents);
  
  return NewDynamicLinkerGroup(linker,
                                section,
                                &linker->code_segment);
}

// The .rela.plt contains the runtime reloations for the .got.plt
// section.
static SectionGroup* AddPLTRelocationsSection(Linker* linker) {
  ELFWriterSectionContents* contents =
  NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* section = NewELFSection(".rela.plt",
                                            SHT(rela),
                                            SHF(alloc),
                                            8, contents);
  section->header.entsize = sizeof(ELFRelocation);
  
  // Allocate space for the relocations but we don't know the
  // contents yet.
  AllocatePLTRelocations(linker, contents);
  
  return NewDynamicLinkerGroup(linker,
                               section,
                               &linker->code_segment);
}

#if 0
static SectionGroup* AddDataRelocationsSection(Linker* linker) {
  ELFWriterSectionContents* contents =
  NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* section = NewELFSection(".rela.data",
                                            SHT(rela),
                                            SHF(alloc),
                                            8, contents);
  section->header.entsize = sizeof(ELFRelocation);

  // Allocate space for the relocations but we don't know the
  // contents yet.
  AllocateDataRelocations(linker, contents);
  
  return NewDynamicLinkerGroup(linker,
                                section,
                                &linker->code_segment);
}
#endif

// The .interp section (and segment) is for finding the dynamic
// loader at runtime.
static SectionGroup* AddInterpreterSection(Linker* linker) {
  ELFWriterSectionContents* contents =
  NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* section = NewELFSection(".interp",
                                            SHT(progbits),
                                            SHF(alloc),
                                            8, contents);

  BufferAppend(&contents->data.buffered,
                linker->interpreter.value,
                linker->interpreter.length + 1);
  
  return NewDynamicLinkerGroup(linker,
                               section,
                               &linker->interpreter_segment);
}

// Write a new ELFDynamicSectionEntry in the given buffer with the
// given value.
static void WriteDynamicSectionEntryWithValue(Buffer* buffer,
                                              ELF_Xword tag,
                                              ELF_Xword value) {
  ELFDynamicSectionEntry entry;
  entry.tag = tag;
  entry.un.val = value;
  BufferAppend(buffer, (char*)&entry, sizeof(entry));
}

// Create the .dynamic section contents for a shared object.  There's
// a chicken-n-egg issue here as this section contains entries whose
// values are offsets into the final file but we don't know what those
// are until the section is created and we have all the information
// to write the file.
static void CreateDynamicSectionContents(Linker* linker, Buffer* buffer) {
  // Write out DT_NEEDED entries
  for (size_t i = 0; i < linker->dynamic_linker->needed_libraries.length; i++) {
    ELF_Word str_offset = (int)linker->dynamic_linker->needed_libraries.value.w[i];
    WriteDynamicSectionEntryWithValue(buffer, DT(needed), str_offset);
  }
  
  // Write out DT_RUNPATH entry.
  if (linker->dynamic_linker->rpath != 0) {
    WriteDynamicSectionEntryWithValue(buffer, DT(runpath), linker->dynamic_linker->rpath);
  }
  
  if (linker->so_name != -1) {
    WriteDynamicSectionEntryWithValue(buffer, DT(soname), linker->so_name);
  }
  
  // Known entries.  If the value is not available until later we
  // write a zero as the entry value here.  It will be overwritten
  // when we know the actual value to use.
  WriteDynamicSectionEntryWithValue(buffer, DT(gnu_hash), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(strtab), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(strsz), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(symtab), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(syment), sizeof(ELFSymbol));
  WriteDynamicSectionEntryWithValue(buffer, DT(rela), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(relasz), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(relacount), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(relaent), sizeof(ELFRelocation));
  
  WriteDynamicSectionEntryWithValue(buffer, DT(pltgot), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(pltrelsz), 0);
  WriteDynamicSectionEntryWithValue(buffer, DT(pltrel), DT(rela));
  WriteDynamicSectionEntryWithValue(buffer, DT(jmprel), 0);

  SectionGroup* preinit = LinkerFindSectionGroup(linker, ".preinit_array");
  if (preinit != NULL && LinkerSectionGroupSize(preinit) > 0) {
    WriteDynamicSectionEntryWithValue(buffer, DT(preinit_array), 0);
    WriteDynamicSectionEntryWithValue(buffer, DT(preinit_arraysz), 0);
  }

  SectionGroup* init_array = LinkerFindSectionGroup(linker, ".init_array");
  if (init_array != NULL && LinkerSectionGroupSize(init_array) > 0) {
    WriteDynamicSectionEntryWithValue(buffer, DT(init_array), 0);
    WriteDynamicSectionEntryWithValue(buffer, DT(init_arraysz), 0);
  }

  SectionGroup* fini_array = LinkerFindSectionGroup(linker, ".fini_array");
  if (fini_array != NULL && LinkerSectionGroupSize(fini_array) > 0) {
    WriteDynamicSectionEntryWithValue(buffer, DT(fini_array), 0);
    WriteDynamicSectionEntryWithValue(buffer, DT(fini_arraysz), 0);
  }

  // TODO: STATIC_TLS flag
  // TODO: text relocations flag.
  
  // Write out DT(null) to terminate section.
  WriteDynamicSectionEntryWithValue(buffer, DT(null), 0);
}

// Given a dynamic section entry tag, replace its value in the buffer
// with the new value.  Returns false if the tag is not present.
static bool FixupDynamicSectionEntryValue(Buffer* buffer,
                                          ELF_Xword tag, ELF_Xword value) {
  size_t index = 0;
  while (index < buffer->length) {
    ELFDynamicSectionEntry* entry =
    (ELFDynamicSectionEntry*)&buffer->value[index];
    if (entry->tag == DT(null)) {
      break;
    }
    if (entry->tag == tag) {
      entry->un.val = value;
      return true;
    }
    index += sizeof(*entry);
  }
  return false;
}

static void FixupArrayDynamicTags(ELFWriterFile* elf, Buffer* buffer,
                                  const char* section_name,
                                  ELF_Xword tag_addr, ELF_Xword tag_size) {
  ELFWriterSection* section = ELFWriterFindSection(elf, section_name);
  if (section == NULL || section->header.size == 0) {
    return;
  }
  FixupDynamicSectionEntryValue(buffer, tag_addr, section->header.addr);
  FixupDynamicSectionEntryValue(buffer, tag_size, section->header.size);
}

// Now that we have the offsets for all the sections we can set the values
// of the entries that were now known when the section was populated.
void DynamicLinkerFixupDynamicSectionContents(ELFWriterFile* elf) {
  ELFWriterSection* dynamic = ELFWriterFindSection(elf, ".dynamic");
  assert(dynamic != NULL);
  
  // Dynamic sections contents buffer.  At this point the section is an
  // output section with mutliple contents, however there is only
  // one section contents in it.
  ELFWriterSectionContents* contents = dynamic->contents->data.multi.value.p[0];
  Buffer* buffer = &contents->data.buffered;
  
  // String table.
  ELFWriterSection* strtab = ELFWriterFindSection(elf, ".dynstr");
  assert(strtab != NULL);
  FixupDynamicSectionEntryValue(buffer, DT(strtab), strtab->header.addr);
  FixupDynamicSectionEntryValue(buffer, DT(strsz), strtab->header.size);
  
  // LinkerSymbol table.
  ELFWriterSection* symtab = ELFWriterFindSection(elf, ".dynsym");
  assert(symtab != NULL);
  FixupDynamicSectionEntryValue(buffer, DT(symtab), symtab->header.addr);
  
  // Relocations.
  ELFWriterSection* rela = ELFWriterFindSection(elf, ".rela.dyn");
  assert(rela != NULL);
  FixupDynamicSectionEntryValue(buffer, DT(rela), rela->header.addr);
  FixupDynamicSectionEntryValue(buffer, DT(relasz), rela->header.size);
  FixupDynamicSectionEntryValue(buffer, DT(relacount),
                                rela->header.size / sizeof(ELFRelocation));
  
  // GNU Hash.
  ELFWriterSection* hash = ELFWriterFindSection(elf, ".gnu_hash");
  assert(hash != NULL);
  FixupDynamicSectionEntryValue(buffer, DT(gnu_hash), hash->header.addr);
  
  // PLT part of the GOT.
  ELFWriterSection* got_plt = ELFWriterFindSection(elf, ".got.plt");
  assert(got_plt != NULL);
  
  FixupDynamicSectionEntryValue(buffer, DT(pltgot),
                                got_plt->header.addr);
  
   // Relocations for GOT PLT entries.
  ELFWriterSection* jmp_rel = ELFWriterFindSection(elf, ".rela.plt");
  assert(jmp_rel != NULL);
  
  // Address of .rela.plt section.
  FixupDynamicSectionEntryValue(buffer, DT(jmprel),
                                jmp_rel->header.addr);
  
  // Size of .rela.plt section.
  FixupDynamicSectionEntryValue(buffer, DT(pltrelsz),
                                jmp_rel->header.size);

  FixupArrayDynamicTags(elf, buffer, ".preinit_array",
                        DT(preinit_array), DT(preinit_arraysz));
  FixupArrayDynamicTags(elf, buffer, ".init_array",
                        DT(init_array), DT(init_arraysz));
  FixupArrayDynamicTags(elf, buffer, ".fini_array",
                        DT(fini_array), DT(fini_arraysz));
}

static SectionGroup* AddDynamicSection(Linker* linker) {
  ELFWriterSectionContents* contents =
     NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* dynamic = NewELFSection(".dynamic",
                                            SHT(dynamic),
                                            SHF(alloc),
                                            8, contents);
  // Set user data to the dynamic_linker info so we can find it
  // during fixup.
  dynamic->user_data = linker->dynamic_linker;
  dynamic->header.entsize = sizeof(ELFDynamicSectionEntry);
  
  // Create the contents of the section.  This will contain values
  // to be fixed up when the information is known.
  CreateDynamicSectionContents(linker, &contents->data.buffered);
  
  return NewDynamicLinkerGroup(linker,
                                dynamic,
                                &linker->dynamic_segment);
}

// Info passed to hash table traversal function for dynamic symbol
// and string tables.
typedef struct  {
  Buffer* dynsym;
  Buffer* dynstr;
} DynamicSymbolTableInfo;

// Since we don't know the section index or value of the symbols
// when they are added to the dynamic symbol table we need to keep
// a reference to the LinkerSymbol inside the symbol table's memory.
// This is so that we can traverse it and insert the actual values
// when we know them.  The union must have a max of sizeof(ELFSymbol)
// bytes.
typedef union {
  ELFSymbol sym;            // 24 bytes.
  struct {
    ELF_Word name_offset;   // 4 bytes.
    uint32_t hash;          // 4 bytes.
    LinkerSymbol* symbol;   // 8 bytes.
  } fixup;                  // Total: 16 bytes.
} SymbolFixup;



// The qsort function doesn't have any way to get user data into the
// comparison function so the only way for it to know the number
// of buckets is to use a static variable.
static int32_t num_gnu_buckets;

// Compare SymbolFixup based on the hash value mod hash table size.
// For GNU hash, the symbol table is sorted by hash bucket.
static int CompareSymbolFixup(const void* a, const void* b) {
  const SymbolFixup* f1 = (const SymbolFixup*)a;
  const SymbolFixup* f2 = (const SymbolFixup*)b;
  return (f1->fixup.hash % num_gnu_buckets) -
           (f2->fixup.hash % num_gnu_buckets);
}

// Sort the dynamic symbol table by hash bucket.  Also, divide the
// symbols into two parts, with undefined symbols before the
// defined ones.  The undefined symbols do not need to be sorted
// by hash bucket.  The division is done by swapping all undefined
// symbols to the start of the table.
//
// We are using the GNU hash table format because it's much
// more modern than the original DT_HASH format.  It requires
// that the symbol table is sorted in the same order as the
// chains in the hash table.  It also contains a symoffset
// that tells it how many symbols are not to be searched.  These
// are the undefined symbols.
static size_t SortDynamicSymbolTable(Buffer* dynsym) {
  assert(num_gnu_buckets > 0);
  size_t num_symbols = dynsym->length / sizeof(SymbolFixup);
  SymbolFixup* symbols = (SymbolFixup*)dynsym->value;
  size_t def_index = -1;    // LinkerSymbol of first defined symbol.
  
  // Find the first defined symbol and set def_index to the
  // index of the first defined symbol.  We start at symbol index
  // 1 since 0 is always the NULL symbol.
  for (size_t i = 1; i < num_symbols; i++) {
    SymbolFixup* symbol = &symbols[i];
    if (symbol->fixup.symbol->header->shndx != 0) {
      def_index = i;
      break;
    }
  }
  
  if (def_index == -1) {
    // No defined symbols so all are undefined.  Return the
    // index of the last symbol.
    return num_symbols - 1;
  }
  
  // Now def_index contains the index of the first symbol in the
  // buffer that is defined.  We now proceed through the table and
  // any symbols found to be undefined are swapped with the last_def
  // index and it is moved on to the next index.
  for (size_t i = def_index + 1; i < num_symbols; i++) {
    SymbolFixup* symbol = &symbols[i];
    if (symbol->fixup.symbol->header->shndx == 0) {
      // Swap undefined symbol with first defined symbol and
      // move the def_index on (since def_index now refers
      // to an undefined symbol).
      SymbolFixup tmp;
      tmp = symbols[i];
      symbols[i] = symbols[def_index];
      symbols[def_index] = tmp;
      def_index++;
    }
  }
  
  // Sort only the defined symbols, which begin at def_index.
  qsort(dynsym->value + def_index * sizeof(SymbolFixup),
        dynsym->length / sizeof(SymbolFixup) - def_index,
        sizeof(SymbolFixup),
        CompareSymbolFixup);
#if 0
  for (size_t i = def_index; i < num_symbols; i++) {
    SymbolFixup* symbol = &symbols[i];
    printf("symbol %s %x is in bucket %d\n", symbol->fixup.symbol->name.value, symbol->fixup.hash, symbol->fixup.hash % num_gnu_buckets);
  }
#endif
  // Return the index of the first defined symbol.
  return def_index;
}

// Add a bucket of global symbols to the dynamic symbol table.
static void AddSymbolListToDynamicSymbolTable(void* entry, void* data) {
  Vector* bucket = entry;
  DynamicSymbolTableInfo* info = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value.p[i];
    // Don't insert symbol if it has no name or it is invented by the
    // linker, like _DYNAMIC_)
    if (sym->name.length == 0 || sym->invented) {
      continue;
    }
    // Add symbol name to string table, getting offset.
    ELF_Word name = (ELF_Word)info->dynstr->length;
    BufferAppend(info->dynstr, sym->name.value, sym->name.length+1);
    
    // Create LinkerSymbol fixup.  This will be replaced by the real
    // symbol when all the information is known. The SymbolFixup
    // struct is the same size as and ELFSymbol.
    SymbolFixup fixup;
    fixup.fixup.name_offset = name;
    fixup.fixup.symbol = sym;
    fixup.fixup.hash = DynamicLoaderGNUHash(sym->name.value);
    BufferAppend(info->dynsym, (char*)&fixup, sizeof(SymbolFixup));
  }
}

// Next power of 2 greater than v.
// See https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static uint64_t NextPowerOf2(uint64_t v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  return v + 1;
}

#if 0
static void DebugPrintHashTable(Buffer* hashtable, size_t num_symbols) {
  DynamicLoaderGNUHashTableHeader* header = (DynamicLoaderGNUHashTableHeader*)hashtable->value;
  printf("nbuckets: %d\n", header->num_buckets);
  printf("symoffset: %d\n", header->symoffset);
  printf("bloom_shift: %d\n", header->bloom_shift);
  printf("bloom_size: %d\n", header->bloom_size);
  uint64_t* bloom_filter = (uint64_t*)(header + 1);
  uint32_t* buckets = (uint32_t*)(bloom_filter + header->bloom_size);
  uint32_t* chains = buckets + header->num_buckets;
  
  for (int i = 0; i < header->bloom_size; i++) {
    printf("bloom[%d] = %" PRIx64 "\n", i, bloom_filter[i]);
  }
  for (int i = 0; i < header->num_buckets; i++) {
    printf("bucket[%d]: %d\n", i, buckets[i]);
  }
  for (int i = 0; i < num_symbols - header->symoffset; i++) {
    printf("chain[%d]: %x\n", i, chains[i]);
  }
}
#endif

static DynamicLoaderGNUHashTableHeader WriteHeader(Buffer* dynsym,
                                                   Buffer* hashtable,
                                                   size_t first_def_index) {
  DynamicLoaderGNUHashTableHeader header;
  header.num_buckets = num_gnu_buckets;
  header.symoffset = (int32_t)first_def_index;
  size_t num_symbols = dynsym->length / sizeof(ELFSymbol);
  
  int64_t num_bits = (num_symbols - first_def_index) * 12;
  header.bloom_size = (int32_t)NextPowerOf2(num_bits / 64);
  header.bloom_shift = 26;
  
  // Write header.
  BufferAppend(hashtable, (char*)&header, sizeof(header));
  return header;
}

static void WriteBloomFilter(Buffer* dynsym,
                             Buffer* hashtable,
                             size_t first_def_index,
                             DynamicLoaderGNUHashTableHeader* header) {
  // Write Bloom filter.
  // Byte index into buffer value for start of bloom filter.
  size_t bloom_index = hashtable->length;
  BufferAddSpace(hashtable, header->bloom_size * sizeof(uint64_t));
  uint64_t* bloom_filter = (uint64_t*)&hashtable->value[bloom_index];
  memset(bloom_filter, 0, header->bloom_size * sizeof(uint64_t));

  size_t num_symbols = dynsym->length / sizeof(ELFSymbol);
  for (size_t i = first_def_index; i < num_symbols; i++) {
    SymbolFixup* sym = &((SymbolFixup*)dynsym->value)[i];
    int index = (sym->fixup.hash / 64) % header->bloom_size;
    
    // Get current bloom filter word and set the bits corresponding
    // to bits 5:0 and bits 31:26 of the hash value.
    bloom_filter[index] |= DynamicLoaderBloomBits64(sym->fixup.hash);
  }
}

static void WriteHashTable(Buffer* dynsym,
                           Buffer* hashtable,
                           size_t first_def_index,
                           DynamicLoaderGNUHashTableHeader* header) {
  // Indexes into hashtable buffer for buckets and chain (byte index).
  size_t bucket_index = hashtable->length;
  size_t chain_index = bucket_index + header->num_buckets * sizeof(uint32_t);

  size_t num_symbols = dynsym->length / sizeof(ELFSymbol);
  size_t num_chains = num_symbols - first_def_index;

  // Allocate space for buckets and chain.  This will ensure that the
  // memory doesn't move and we can simply use pointers into the
  // buffer data to set the values.
  BufferAddSpace(hashtable,
                 (header->num_buckets + num_chains) * sizeof(uint32_t));
  
  // Pointers to buckets and chains.  These address memory inside
  // the hashtable buffer, which won't move because we are not
  // adding anything to it now.
  uint32_t* buckets = (uint32_t*)&hashtable->value[bucket_index];
  uint32_t* chains = (uint32_t*)&hashtable->value[chain_index];
  
  int curr_bucket = 0;
  buckets[0] = (int32_t)first_def_index;
  int last_chain = -1;
  
  for (size_t i = first_def_index; i < num_symbols; i++) {
    SymbolFixup* sym = &((SymbolFixup*)dynsym->value)[i];
    // Value to insert into chain entry is the hash with the
    // bottom bit cleared.
    uint32_t hash = sym->fixup.hash & ~1;
#if 0
    printf("adding %s to hash with value %x bucket %d\n",
           sym->fixup.symbol->name.value, hash, curr_bucket);
#endif
    // Moving to next bucket?
    if ((sym->fixup.hash % num_gnu_buckets) != curr_bucket) {
      if (last_chain != -1) {
        // Set bottom bit of last chain entry set.
        chains[last_chain] |= 1;
      }
      curr_bucket = sym->fixup.hash % num_gnu_buckets;
      buckets[curr_bucket] = (uint32_t)i;
    }
    
    // Add chain entry.
    last_chain = (int)(i - first_def_index);
    chains[last_chain] = hash;
  }
  
  // Set bottom bit of last chain entry.
  if (last_chain != -1) {
    chains[last_chain] |= 1;
  }
}

// A 2-bit Bloom filter is used to quickly determine the presence of a
// hash value in the table.  Apparently this make a lot of difference
// to the speed (saves over 80% of string comparisons over the old hash
// system).
//
// The llvm system uses 12 bits per symbol in the bloom filter.  Two
// parts of the hash value are used to set the bits of a single bloom
// filter word: bits 5:0 and bits 31:26.
//
// This for 64 bits only.
//
// This must be done before the fixups for the dynamic symbol
// table are done as it relies on the 'fixup' member of the
// union.
static void CreateGNUHashTable(Buffer* dynsym,
                               Buffer* hashtable,
                               size_t first_def_index) {
  // Create hash table header and write it.
  DynamicLoaderGNUHashTableHeader header = WriteHeader(
                                                       dynsym,
                                                       hashtable,
                                                       first_def_index);
  // Write Bloom filter.
  WriteBloomFilter(dynsym, hashtable, first_def_index, &header);
  
  // Write the hash table buckets and chains.
  WriteHashTable(dynsym, hashtable, first_def_index, &header);

#if 0
  DebugPrintHashTable(hashtable,
                      dynsym->length / sizeof(ELFSymbol));
#endif
}

// We know the address and section indexes for the symbols now, so go
// through the dynamic symbol table and apply the information to the
// ELFSymbols held therein.
void DynamicLinkerFixupDynamicSymbolTable(Buffer* dynsym,
                                    int32_t bss_section_index) {
  // Byte index into symbol table.
  size_t index = sizeof(SymbolFixup);
  while (index < dynsym->length){
    SymbolFixup* fixup = (SymbolFixup*)&dynsym->value[index];
    LinkerSymbol* sym = fixup->fixup.symbol;
    ELFSymbol* elfsym = &fixup->sym;
    int32_t type = ELF_ST_TYPE(sym->header->info);
    int32_t binding = ELF_ST_BIND(sym->header->info);
    ELFSymbolInit(elfsym, fixup->fixup.name_offset,
                  sym->header->shndx,
                  type, binding, sym->header->size,
                  sym->address);
    
    // Set the index into the dynamic symbol table in the LinkerSymbol.
    // This is used by the dynamic relocations.
    sym->dynamic_index = (int)index / sizeof(SymbolFixup);
    
    index += sizeof(SymbolFixup);
  }
}

static char* Basename(String* pathname) {
  char* leaf = strrchr(pathname->value, '/');
  if (leaf != NULL) {
    leaf++;     // Skip /.
  } else {
    leaf = pathname->value;   // Whole name.
  }
  return leaf;
}

static void InsertDynamicStrings(Linker* linker, ELFWriterSection* strtab,
                                 ELFWriterSectionContents* strtab_contents) {
  // Write so_name as the second entry in the dynstr.
  linker->so_name = (int)strtab->contents->data.buffered.length;
  char* basename = Basename(&linker->output_filename);
  BufferAppend(&strtab_contents->data.buffered,
               basename,
               strlen(basename)+1);
  
  // We always need libc.so, so add that now.
#if 0
  const char libc[] = "libc.so";
  VectorAppend(&linker->dynamic_linker->needed_libraries,
               (void*)strtab_contents->data.buffered.length);
  BufferAppend(&strtab_contents->data.buffered,
               (char*)libc,
               sizeof(libc));
#endif
  
  // Now add all the names for the dynamic libraries to the string
  // table, recording their offsets in the needed_libraries vector.
  for (size_t i = 0; i < linker->dynamic_libraries.length; i++) {
    LoadedDynamicLibrary* lib = linker->dynamic_libraries.value.p[i];
    VectorAppend(&linker->dynamic_linker->needed_libraries,
                 (void*)strtab_contents->data.buffered.length);
    // Add final leaf filename to the NEEDED libraries.
     char* basename = Basename(&lib->libname);
     BufferAppend(&strtab_contents->data.buffered, basename,
                 strlen(basename) + 1);
  }
  
  // Add rpath
  String rpath;
  StringInit(&rpath, NULL);
  char* sep = "";
  for (size_t i = 0; i < linker->rpath.length; i++) {
    String* path = linker->rpath.value.p[i];
    StringAppend(&rpath, sep);
    StringAppend(&rpath, path->value);
    sep = ":";
  }
  linker->dynamic_linker->rpath = strtab_contents->data.buffered.length;
  BufferAppend(&strtab_contents->data.buffered, rpath.value,
               rpath.length + 1);
  StringDestruct(&rpath);
}

// Create the dynamic symbol and string tables.  Returns the
// string table since we need that again.  Adds the section
// groups to the code segment.
static void AddDynamicSymbolTable(Linker* linker) {
  ELFWriterSectionContents* strtab_contents =
    NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* strtab = NewELFSection(".dynstr",
                                           SHT(strtab),
                                           SHF(alloc) | SHF(strings),
                                           8, strtab_contents);
  // Write NULL symbol name as the first entry in dynstr.
  BufferAppend(&strtab_contents->data.buffered, "", 1);
  
  InsertDynamicStrings(linker, strtab, strtab_contents);
  
  ELFWriterSectionContents* symtab_contents =
  NewELFWriterSectionContents(kSectionContentsBuffered);
  ELFWriterSection* symtab = NewELFSection(".dynsym",
                                           SHT(dynsym),
                                           SHF(alloc),
                                           8, symtab_contents);
  symtab->header.entsize = sizeof(ELFSymbol);
  
  NewDynamicLinkerGroup(linker, symtab,
                        &linker->code_segment);

  // Traverse the global symbol table, adding all symbols to the dynamic
  // symbol and string tables.
  DynamicSymbolTableInfo info = {
    .dynsym = &symtab_contents->data.buffered,
    .dynstr = &strtab_contents->data.buffered,
  };
  
  // Add NULL symbol to the start of the dynamic symbol table.
  SymbolFixup null_symbol;
  memset(&null_symbol.sym, 0, sizeof(null_symbol));
  BufferAppend(info.dynsym, (char*)&null_symbol, sizeof(SymbolFixup));

  HashTableTraverse(&linker->global_symbol_table,
                    AddSymbolListToDynamicSymbolTable, &info);
  
  NewDynamicLinkerGroup(linker,
                        strtab,
                        &linker->code_segment);

  // GNU hash table.
  ELFWriterSectionContents* gnu_hash_contents =
    NewELFWriterSectionContents(kSectionContentsBuffered);
  Buffer* hashtable = &gnu_hash_contents->data.buffered;
  
  ELFWriterSection* gnu_hashtable = NewELFSection(".gnu_hash",
                                           SHT(gnu_hash),
                                           SHF(alloc),
                                           8, gnu_hash_contents);
  
  NewDynamicLinkerGroup(linker, gnu_hashtable,
                        &linker->code_segment);

  // Number of buckets in the GNU hash table, chosen to give 4 symbols
  // per bucket, assuming an even distribution.
  // NOTE: this is a static variable, defined above the sort function.
  num_gnu_buckets = (int32_t)linker->global_symbol_table.object_count / 4;
  if (num_gnu_buckets == 0) {
    num_gnu_buckets = 1;
  }
  
  // Sort the symbol table by hash buckets.
  size_t first_def_index = SortDynamicSymbolTable(info.dynsym);
  
  // Create the GNU hash table section and group.
  CreateGNUHashTable(info.dynsym, hashtable, first_def_index);
  
  // Align buffer sizes.
  BufferAlignLength(info.dynsym, 8);
  BufferAlignLength(info.dynstr, 8);
  BufferAlignLength(hashtable, 8);
}


// Create all the sections groups for the dynamic library
// sections.  Why groups?  The linker groups all the sections
// it links together into a number of SectionGroup structs.  A
// SectionGroup contains a set of components that will all end
// up in the same section in the output file.  Even if there is
// only one section in the group, we stil need to have the
// group.
void DynamicLinkerCreateDynamicLinkerGroups(Linker* linker) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  // Add dynamic symbol table and get string table.
  AddDynamicSymbolTable(linker);
  
  // Add the global offset table.
  AddGlobalOffsetTable(linker, &dynamic->got_group, &dynamic->got_plt_group);
  
  // Add the procedure linkage table.
  dynamic->plt_group = AddProcedureLinkageTable(linker);
  
  // Add the dynamic section.
  dynamic->dynamic_group = AddDynamicSection(linker);
  
  // Add dynamic relocations.
  dynamic->dyn_rela_group = AddDynamicRelocationsSection(linker);

  // Add PLT relocations.
  dynamic->plt_rela_group = AddPLTRelocationsSection(linker);

  // Interpreter.
  if (!linker->building_dso) {
    dynamic->interpreter_group = AddInterpreterSection(linker);
  }
}
