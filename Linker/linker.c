//
//  linker.c
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker.h"
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include <assert.h>
#include "elf_reader.h"
#include "linker_reloc.h"
#include "linker_symbols.h"
#include "linker_file.h"
#include "linker_dynamic.h"
#include <sys/stat.h>

void LinkerError(LinkerFile* file, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  VReportError(filename, 0,
               error, ap);
  va_end(ap);
}

void VLinkerError(LinkerFile* file, const char* error, va_list ap) {
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  VReportError(filename, 0,
               error, ap);

}

void LinkerWarning(LinkerFile* file, const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  VReportWarning(filename, 0,
                 warn, error, ap);
  va_end(ap);

}

void VLinkerWarning(LinkerFile* file, const char* warn, const char* error, va_list ap) {
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  VReportWarning(filename, 0,
                 warn, error, ap);
}



void LinkerInit(Linker* linker) {
  VectorInit(&linker->files);
  HashTableInit(&linker->global_symbol_table, "global-symbol-table", 1009,
                LinkerSymbolHash, LinkerSymbolInsertInHashTable, LinkerSymbolFindInHashTable);
  VectorInit(&linker->section_groups);
  VectorInit(&linker->library_search_path);
  VectorInit(&linker->static_libraries);
  VectorInit(&linker->dynamic_libraries);

  // Create the initial library search path.
  VectorAppend(&linker->library_search_path, NewString("/usr/lib"));
  VectorAppend(&linker->library_search_path, NewString("/lib"));
  linker->elf_machine_type = 0;
  linker->elf_flags = 0;
  linker->dso = false;
  
  // Add the contents of LD_LIBRARY_PATH to the library search path.
  char* ld_library_path = getenv("LD_LIBRARY_PATH");
  if (ld_library_path != NULL) {
    char* start = ld_library_path;
    while (*start != '\0') {
      char* p = ld_library_path;
      while (*p != ':' && *p != '\0') {
        p++;
      }
      String* dir = NewString("");
      StringAppendSegment(dir, start, p - start);
      VectorAppend(&linker->library_search_path, dir);
      start = p;
      if (*start == ':') {
        start++;
      }
    }
  }
}


void LinkerDestruct(Linker* linker) {
  VectorDestructWithContents(&linker->files, (VectorElementDestructor)LinkerFileDestruct);
  LinkerClearSymbolTable(&linker->global_symbol_table);
  HashTableDestruct(&linker->global_symbol_table);
  VectorDestructWithContents(&linker->section_groups, (VectorElementDestructor)SectionGroupDestruct);
  VectorDestructWithContents(&linker->static_libraries, (VectorElementDestructor)ARArchiveDestruct);
  VectorDestructWithContents(&linker->dynamic_libraries, (VectorElementDestructor)DynamicLibraryDestruct);
  VectorDestructWithContents(&linker->library_search_path, (VectorElementDestructor)StringDestruct);
}

void LinkerAddLibrarySearchDir(Linker* linker, String* dir) {
  VectorAppend(&linker->library_search_path, dir);
}

// Find a library in the search path given its name and a file suffix.  The filename
// will be formed by prepending "lib" and appending the suffix (.a or .so).  Returns
// true if the library was found, in which case the filename will be set the full path
// name of the file.
static bool FindLibrary(Linker* linker, const char* name, const char* suffix, String* filename) {
  // Search the library search path for the library with the prefix
  // "lib" and the suffix.
  bool found = false;
  for (size_t i = 0; i < linker->library_search_path.length; i++) {
    String pathname;
    String* dir = linker->library_search_path.value[i];
    StringInit(&pathname, "");
    StringPrintf(&pathname, "%s/lib%s%s", dir->value, name, suffix);
    struct stat st;
    int e = stat(pathname.value, &st);
    if (e == 0) {
      // Found.
      StringSet(filename, pathname.value);
      found = true;
      break;
    }
    StringDestruct(&pathname);
  }
  return found;
}

void LinkerAddStaticLibrary(Linker* linker, const char* name) {
  ARArchive* archive = NewARArchive(name);
  FILE* fp = fopen(name, "r");
  if (fp == NULL) {
    return;
  }
  
  bool ok = ARArchiveOpen(archive, fp);
  if (!ok) {
    return;
  }
  VectorAppend(&linker->static_libraries, archive);
  fclose(fp);
}

void LinkerAddDynamicLibrary(Linker* linker, const char* name) {
  String filename;
  StringInit(&filename, name);
  LinkerFile* file = LinkerReadDynamicObject(linker, &filename);
  if (file == NULL) {
    StringDestruct(&filename);
    return;
  }
  DynamicLibrary* lib = NewDynamicLibrary(file);
  VectorAppend(&linker->dynamic_libraries, lib);
  StringDestruct(&filename);
}

bool LinkerAddLibrary(Linker* linker, const char* name) {
  String filename;
  StringInit(&filename, "");
  bool found = FindLibrary(linker, name, ".a", &filename);
  if (found) {
    LinkerAddStaticLibrary(linker, filename.value);
    return true;
  }
  found = FindLibrary(linker, name, ".so", &filename);
  if (found) {
    LinkerAddDynamicLibrary(linker, filename.value);
    return true;
  }
  return false;
}

bool LinkerFindSymbolInStaticLibraries(Linker* linker, const char* name,
                                 ARArchive** archive,
                                 ARFile** file) {
  for (size_t i = 0; i < linker->static_libraries.length; i++) {
    *archive = linker->static_libraries.value[i];
    ARSymbol* sym = ARArchiveFindSymbol(*archive, name);
    if (sym != NULL) {
      *file = sym->file;
      return true;
    }
  }
  return false;
}

LinkerSymbol* LinkerFindSymbol(HashTable* symbol_table,
                               const char* name) {
  return HashTableSearch(symbol_table, (void*)name);
}

void LinkerInsertSymbol(HashTable* symbol_table,
                        LinkerSymbol* sym) {
  HashTableInsert(symbol_table, sym);
}

SectionGroup* NewSectionGroup(const String* name, int32_t type, int64_t flags, int64_t alignment) {
  SectionGroup* group = malloc(sizeof(SectionGroup));
  StringInit(&group->name, name->value);
  VectorInit(&group->components);
  group->type = type;
  group->flags = flags;
  group->alignment = alignment;
  group->segment = NULL;
  group->address = 0;
  return group;
}

void SectionGroupDestruct(SectionGroup* group) {
  StringDestruct(&group->name);
  VectorDestruct(&group->components);
}

void SectionGroupDelete(SectionGroup* group) {
  SectionGroupDestruct(group);
  free(group);
}

void SegmentInit(Segment* segment) {
  VectorInit(&segment->sections);
  segment->address = 0;
}

void SegmentDestruct(Segment* segment) {
  VectorDestruct(&segment->sections);
}

// Data for passing to map traverse function to build the section groups.
struct SectionGroupingData {
  Linker* linker;
  int32_t section_type;
};

static void ReadELFContents(Linker* linker, ELFReaderFile* elf_file,
                            LinkerFile* file) {
  // Build the sections_by_name map in the LinkerFile.  This is a map
  // whose key is the section name (String*) and the value is and ELFReaderSection*.
  // Also build the sections_by_type map.
  for (size_t i = 0; i < elf_file->sections.length; i++) {
    ELFReaderSection* section = elf_file->sections.value[i];
    MapInsert(&file->sections_by_name, &section->name, section);
    
    Vector* type_list = MapFind(&file->sections_by_type, (void*)(int64_t)(section->header->type));
    if (type_list == NULL) {
      type_list = NewVector();
      MapInsert(&file->sections_by_type, (void*)(int64_t)(section->header->type), type_list);
    }
    VectorAppend(type_list, section);
  }
  
  // Build the symbol tables.  There is probably only one in the file
  // but we should find them all just in case.  Each symbol table section
  // says where its string table is in the 'link' field in the header.
  Vector symbol_tables;
  VectorInit(&symbol_tables);
  ELFReaderFileFindSectionsByType(elf_file, SHT(symtab), &symbol_tables);
  
  for (size_t sect = 0; sect < symbol_tables.length; sect++) {
    ELFReaderSection* symtab = symbol_tables.value[sect];
    if (symtab->header->link >= elf_file->sections.length) {
      LinkerError(file, "Corrupt symbol table link value");
      continue;
    }
    ELFReaderSection* strtab = elf_file->sections.value[symtab->header->link];
    size_t num_symbols = symtab->header->size / symtab->header->entsize;
    const char* symbol_addr = (const char*)elf_file->header + symtab->header->offset;
    
    // Now read the symbols and add them to the symbol tables in the file.
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol* elf_sym = (ELFSymbol*)symbol_addr;
      LinkerReadSymbol(linker, file, elf_file, symtab, strtab, elf_sym);
      symbol_addr += symtab->header->entsize;
    }
  }
  VectorDestruct(&symbol_tables);
  
  // Build the relocation tables.
  Vector relocation_sections;
  VectorInit(&relocation_sections);
  
  // Find all relocatation sections.  Finds both REL and RELA sections.
  ELFReaderFileFindSectionsByType(elf_file, SHT(rela), &relocation_sections);
  ELFReaderFileFindSectionsByType(elf_file, SHT(rel), &relocation_sections);
  
  // Process all relocation sections.
  for (size_t i = 0; i < relocation_sections.length; i++) {
    ELFReaderSection* reloc_section = relocation_sections.value[i];
    int32_t symtab_section_index = reloc_section->header->link;
    // TODO: look up the relocated section here rather than in
    // LinkerReadRelocation.
    if (symtab_section_index >= elf_file->sections.length) {
      LinkerError(file, "Corrupt relocation symbol table section index");
      continue;
    }
    ELFReaderSection* symtab = elf_file->sections.value[symtab_section_index];
    
    if (symtab->header->link >= elf_file->sections.length) {
      LinkerError(file, "Corrupt relocation symbol section index");
      continue;
    }
    ELFReaderSection* strtab = elf_file->sections.value[symtab->header->link];
    const char* symbol_table_address = (const char*)elf_file->header +
    symtab->header->offset;
    
    int64_t num_relocations = reloc_section->header->size /
    reloc_section->header->entsize;
    
    const char* reloc_addr = (const char*)elf_file->header +
    reloc_section->header->offset;
    for (int64_t ri = 0; ri < num_relocations; ri++) {
      ELFRelocation* reloc = (ELFRelocation*)reloc_addr;
      
      LinkerReadRelocation(linker, file, elf_file, reloc, symbol_table_address,
                           reloc_section, symtab, strtab);
      reloc_addr += reloc_section->header->entsize;
    }
  }
  VectorDestruct(&relocation_sections);
}

static bool CheckMachineType(Linker* linker, LinkerFile* file) {
  if (linker->files.length == 1) {
    // First file, record machine type.
    linker->elf_machine_type = file->elf_file->header->machine;
    linker->elf_flags = file->elf_file->header->flags;
  } else {
    if (linker->elf_machine_type != file->elf_file->header->machine) {
      LinkerError(file, "Inconsistent ELF machine type");
      return false;
    }
  }
  return true;
}

// Read a complete ELF file and add the symbol tables and relocations
// to the linker's internal data.
bool LinkerReadObjectFile(Linker* linker, String* filename) {
  ELFReaderFile* elf_file = NewELFReaderFile(filename);
  LinkerFile* file = NewLinkerFile(elf_file, linker, filename->value);
  VectorAppend(&linker->files, file);
  bool ok = ELFReaderFileRead(elf_file, 0, 0);
  if (!ok) {
    ELFReaderFileDelete(elf_file);
    return false;
  }
  if (!CheckMachineType(linker, file)) {
    ELFReaderFileDelete(elf_file);
    return false;
  }

  ReadELFContents(linker, elf_file, file);
  return true;
}

bool LinkerReadObjectFileFromArchive(Linker* linker, ARArchive* archive,
                                     ARFile* ar_file) {
  ELFReaderFile* elf_file = NewELFReaderFile(&archive->filename);
  LinkerFile* file = NewLinkerFile(elf_file, linker, ar_file->filename.value);
  VectorAppend(&linker->files, file);
  
  bool ok = ELFReaderFileRead(elf_file, ar_file->size, ar_file->file_offset);
  if (!ok) {
    ELFReaderFileDelete(elf_file);
    return false;
  }
  
  if (!CheckMachineType(linker, file)) {
    ELFReaderFileDelete(elf_file);
    return false;
  }

  ReadELFContents(linker, elf_file, file);
  return true;
}

// Read a dynamic object, returning the LinkerFile holding its contents.
LinkerFile* LinkerReadDynamicObject(Linker* linker, String* filename) {
  ELFReaderFile* elf_file = NewELFReaderFile(filename);
  bool ok = ELFReaderFileRead(elf_file, 0, 0);
  if (!ok) {
    ELFReaderFileDelete(elf_file);
    return NULL;
  }
  LinkerFile* file = NewLinkerFile(elf_file, linker, filename->value);
  if (!CheckMachineType(linker, file)) {
    ELFReaderFileDelete(elf_file);
    LinkerFileDelete(file);
    return NULL;
  }
  ReadELFContents(linker, elf_file, file);
  return file;
}

// Check if a symbol table bucket contains undefined symbola
// and if so, look for them in the libraries and if found
// link in the library file defining them.
static void ResolveUndefined(void* entry, void* data) {
  Vector* bucket = entry;
  Linker* linker = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value[i];
    if (!symbol->defined) {
      ARArchive* archive;
      ARFile* file;
      bool found = LinkerFindSymbolInStaticLibraries(linker, symbol->name.value,
                                               &archive, &file);
      if (found) {
        LinkerReadObjectFileFromArchive(linker, archive, file);
      }
      
      // TODO: look in dynamic libraries.  If it is found, mark it as
      // resolved externally but don't link in the library.  It will
      // be done at runtime.
    }
  }
}

// Resolve all undefined symbols in the libraries.
static void ResolveUndefinedSymbols(Linker* linker) {
  HashTableTraverse(&linker->global_symbol_table, ResolveUndefined, linker);
}

static void BuildSectionGroup(const void* key, void* value, void* data) {
  const String* name = key;
  Vector* sections = value;
  Linker* linker = ((struct SectionGroupingData*)data)->linker;
  int32_t section_type = ((struct SectionGroupingData*)data)->section_type;

  // Take the flags and alignment from the first section.
  ELFReaderSection* first_section = sections->value[0];
  int64_t section_flags = first_section->header->flags;
  int64_t section_alignment = first_section->header->addralign;

  SectionGroup* group = NewSectionGroup(name, section_type, section_flags,
                                        section_alignment);
  VectorAppend(&linker->section_groups, group);

  VectorCopy(&group->components, sections);
}

// Print a section map key/value pair.
static void PrintSectionMapKV(const void* key, void* value, void* data) {
  const String* name = key;
  const Vector* sections = value;
  printf("Name: %s\n", name->value);
  for (size_t i = 0; i < sections->length; i++) {
    ELFReaderSection* section = sections->value[i];
    printf("  [%zd]: %s %p @%llx\n", i, section->name.value, section->contents, section->address);
  }
}

static void PrintSectionMap(Map* map) {
  MapTraverse(map, PrintSectionMapKV, NULL);
}

// Map compare function for section names.  The key is a String*.
static int CompareMappedSectionNames(const void*a, const void* b) {
  const MapKeyValue* s1 = a;
  const MapKeyValue* s2 = b;
  return StringCompareString(s1->key, s2->key);
}

// Group all sections with the given type into a the section_groups
// vector in the Linker.
static void GroupSections(Linker* linker, int32_t section_type) {
  Map section_map;
  MapInit(&section_map, CompareMappedSectionNames);

  // Build a map of section name vs vectors of pointers to ELFReaderSections
  // with the type given.  A traversal will be in alphabetic order by
  // section name.
  for (size_t i = 0; i < linker->files.length; i++) {
    LinkerFile* file = linker->files.value[i];

    Vector* sections = MapFind(&file->sections_by_type,
                               (void*)((int64_t)(section_type)));
    if (sections == NULL) {
      continue;
    }
    for (size_t j = 0; j < sections->length; j++) {
      ELFReaderSection* section = sections->value[j];
      Vector* result_vec = MapFind(&section_map, &section->name);
      if (result_vec == NULL) {
        result_vec = NewVector();
        MapInsert(&section_map, &section->name, result_vec);
      }
      VectorAppend(result_vec, section);
    }
  }

  PrintSectionMap(&section_map);
  
  // The section_map contains a mapping of section name vs a vector of
  // ELFReaderSection pointers (all the sections with that name within the
  // files).
  //
  // Now convert the map to a Vector of SectionGroups inside the Linker.
  struct SectionGroupingData data = {
    .linker=linker,
    .section_type=section_type,
  };
  MapTraverse(&section_map, BuildSectionGroup, &data);

  // Now we have the sections grouped we can assign each section to its
  // appropriate segment.  The assignment is done by use of the flags
  // in the SectionGroup.  Any SectionGroup that is read-only is placed
  // in the code segment.  If it is writeable is is place in the data segment.
  for (size_t i = 0; i < linker->section_groups.length; i++) {
    SectionGroup* group = linker->section_groups.value[i];
    Segment* segment = &linker->code_segment;
    if ((group->flags & SHF(write)) != 0) {
      segment = &linker->data_segment;
    }
    group->segment = segment;
    VectorAppend(&segment->sections, group);
  }

  // We don't need this section map now that we have the section groups.
  MapDestruct(&section_map);
}

static void AddDynamicSections(Linker* linker) {
  
}

// Assign addresses to all the sections held within the segment.  The starting address is
// passed and the final address is returned.
static uint64_t AssignSegmentSectionAddresses(Segment* segment, uint64_t address) {
  for (size_t i = 0; i < segment->sections.length; i++) {
    SectionGroup* group = segment->sections.value[i];
    group->address = address;

    // Concatenate all the component sections, assigning
    // consecutive addresses.
    for (size_t j = 0; j < group->components.length; j++) {
      ELFReaderSection* section = group->components.value[j];
      section->address = address;
      address += section->header->size;
    }
  }
  return address;
}

static ELFReaderSection* FirstSectionInSegment(Segment* segment) {
  assert(segment->sections.length > 0);
  SectionGroup* group = segment->sections.value[0];
  assert(group->components.length > 0);
  return group->components.value[0];
}

static ELFReaderSection* LastSectionInSegment(Segment* segment) {
  assert(segment->sections.length > 0);
  SectionGroup* group = segment->sections.value[segment->sections.length-1];
  assert(group->components.length > 0);
  return group->components.value[group->components.length-1];
}

// Link all files passed to the linker together.  This gathers the sections
// with the same names into the same place and assigns addresses to the
// sections and symbols.
void LinkerLinkAllFiles(Linker* linker) {
  SegmentInit(&linker->code_segment);
  SegmentInit(&linker->data_segment);

  // Resolve all undefined symbols in libraries.
  ResolveUndefinedSymbols(linker);
  
  // Find all PROGBITS sections and group by name.  These are sections
  // that have data associated with them in the ELF file.
  GroupSections(linker, SHT(progbits));

  // If are building a DSO, add the generated dynamic sections to the segments.
  AddDynamicSections(linker);
  
  // Assign addresses to all sections.
  // This variable is updated as we assign the addresses to the sections.
  // When the traversal is complete it will contain the address after the
  // last section.
  uint64_t current_address = LINKER_CODE_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET;

  // We know how many sections there are now.  This is the number of groups + the number
  // of extra sections we add.  Add space for the section headers ,each of whick is
  // sizeof(ELFSectionHeader) bytes long.
  // We also are going to create a BSS section.
  current_address += (linker->section_groups.length +
                      LINKER_NUM_EXTRA_SECTIONS(linker) + 1) * sizeof(ELFSectionHeader);

  // Assign code segment addresses.
  uint64_t code_segment_end = AssignSegmentSectionAddresses(&linker->code_segment, current_address);
  uint64_t code_segment_length = code_segment_end - current_address;

  // And now the data segment addresses.
  current_address = LINKER_DATA_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET;
  current_address += (linker->section_groups.length +
                      LINKER_NUM_EXTRA_SECTIONS(linker) + 1) * sizeof(ELFSectionHeader) + code_segment_length;

  current_address = AssignSegmentSectionAddresses(&linker->data_segment, current_address);

  // Now that we know the addresses of the sections we can work
  // out the values of the symbols within those sections.
  LinkerAssignSymbolAddresses(linker);
  
  // Assign section symbol addresses.
  LinkerAssignSectionSymbolAddresses(linker);
  
  // The .bss (nobits) address is just after all the progbits sections.
  linker->nobits_address = current_address;

  LinkerAssignCommonSymbolAddresses(linker, &current_address);
  linker->nobit_size = current_address - linker->nobits_address;
  LinkerAssignBSSSymbolAddresses(linker);

  LinkerPrintSymbolTables(linker);

  // We have all the values of the symbols, apply those values to
  // all the relocations in the files.
  LinkerApplyAllRelocations(linker);

  if (!linker->dso) {
    // Check if we have any undefined symbols and report errors if found.
    LinkerCheckForUndefinedSymbols(linker);
  }
}

static ELFWriterSection* BuildOutputSection(ELFWriterFile* elf, SectionGroup* group) {
  ELFWriterSectionContents* contents = NewELFWriterSectionContents(kSectionContentsMulti);
  ELFWriterSection* section = ELFWriterAddSection(elf, &group->name,
                      group->type,
                      group->flags,
                      group->alignment,
                      contents, group->address);

  for (size_t i = 0; i < group->components.length; i++) {
    ELFReaderSection* part = group->components.value[i];
    ELFWriterSectionContents* part_contents = NewELFWriterSectionContents(kSectionContentsRaw);
    part_contents->size = part->header->size;
    part_contents->data.raw = part->contents;
    VectorAppend(&contents->data.multi, part_contents);
  }
  return section;
}

static void AssignGroupSectionIndexes(SectionGroup* group, int32_t* index_ptr) {
  for (size_t i = 0; i < group->components.length; i++) {
    ELFReaderSection* section = group->components.value[i];
    section->output_section_index = *index_ptr;
  }
  // Move the index on.
  (*index_ptr)++;
}

// Add a bucket of symbols to the output ELF file.
static void AddSymbolListToOutput(void* entry, void* data) {
  Vector* bucket = entry;
  ELFWriterFile* elf = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value[i];
    int32_t type = ELF_ST_TYPE(sym->header->info);
    int32_t binding = ELF_ST_BIND(sym->header->info);
    int32_t sym_index;
    int32_t section_index;
    if (sym->section == NULL) {
      // Common symbol. This is in the BSS section.  The index
      // of this is determined from the number of sections
      // in the ELF file.  The BSS section is the last one added
      // to the file (before the symbol table, string table, etc.)
      section_index = (int32_t)elf->sections.length - 1;
    } else {
      section_index = sym->section->output_section_index;
    }
    ELFWriterAddSymbol(elf, &sym->name, section_index,
                       type, binding, sym->header->size,
                       sym->address, &sym_index);
  }
}

// Write the output file.
void LinkerWriteOutput(Linker* linker, FILE* output) {
  ELFWriterFile elf;

  ELFWriterFileInit(&elf,
                    linker->dso ? ET(dyn) : ET(exec),
                    linker->elf_machine_type,
                    linker->elf_flags, linker->dso);

  if (!linker->dso) {
    // Find "main".  This is the entry point for the program.
    LinkerSymbol* main = LinkerFindSymbol(&linker->global_symbol_table, "main");
    if (main == NULL) {
      LinkerError(NULL, "No main function found");
      return;
    }
    elf.header.entry = main->address;
  }
  
  // Add NULL section.
  ELFWriterSectionContents* null_contents = NewELFWriterSectionContents(kSectionContentsRaw);
  ELFWriterAddSection(&elf, NULL,
                      SHT(null), 0, 8, null_contents, 0);

  // Build output sections in code segment.
  for (size_t i = 0; i < linker->code_segment.sections.length; i++) {
    BuildOutputSection(&elf, linker->code_segment.sections.value[i]);
  }

  // Build output sections in data segment.
  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    BuildOutputSection(&elf, linker->data_segment.sections.value[i]);
  }

  // Add BSS section.
  ELFWriterSectionContents bss_contents;
  ELFWriterSectionContentsInit(&bss_contents, kSectionContentsNobits);
  bss_contents.size = linker->nobit_size;
  String bss_name;
  StringInit(&bss_name, ".bss");
  ELFWriterAddSection(&elf, &bss_name,
                      SHT(nobits),
                      SHF(alloc) | SHF(write),
                      8,
                      &bss_contents, linker->nobits_address);

  // Assign the section indexes for the output sections.
  int32_t section_index = 1;
  for (size_t i = 0; i < linker->code_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->code_segment.sections.value[i], &section_index);
  }

  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->data_segment.sections.value[i], &section_index);
  }
  
  // Add symbols.
  HashTableTraverse(&linker->global_symbol_table, AddSymbolListToOutput, &elf);

  // Now build the code and data segments in the ELF output file.
  ELFWriterSegment* code_segment = NewELFWriterSegment(PT(load), PF(r) | PF(x), LINKER_SEGMENT_ALIGNMENT);
  ELFWriterSegment* data_segment = NewELFWriterSegment(PT(load), PF(r) | PF(w), LINKER_SEGMENT_ALIGNMENT);

  // We have added all output sections to the ELF file.  Now we can add them all to the
  // code and data segments in the ELF file.
  // NOTE: the first section is a NULL section so we don't count that.
  for (size_t i = 1; i < elf.sections.length; i++) {
    ELFWriterSection* section = elf.sections.value[i];
    if ((section->header.flags & SHF(write)) == 0) {
      ELFWriterSegmentAddSection(code_segment, section);
    } else {
      ELFWriterSegmentAddSection(data_segment, section);
    }
  }
  VectorAppend(&elf.segments, code_segment);
  VectorAppend(&elf.segments, data_segment);

  ELFWriterFileWrite(&elf, output);
  ELFWriterFileDestruct(&elf);
  StringDestruct(&bss_name);
}

