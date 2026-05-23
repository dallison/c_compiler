//
//  linker.c
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker.h"
#include "linker_arch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "errors.h"
#include <assert.h>
#include "elf_reader.h"
#include "linker_reloc.h"
#include "linker_symbols.h"
#include "linker_file.h"
#include "linker_dynamic.h"
#include <sys/stat.h>

// Supported architectures.
#include "linker_arch_pcode.h"
#include "linker_arch_riscv.h"
#include "linker_arch_6502.h"
#include "linker_arch_aarch64.h"

void LinkerError(ObjectFile* file, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLinkerError(file, error, ap);
  va_end(ap);
}

void VLinkerError(ObjectFile* file, const char* error, va_list ap) {
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  char buf[1024];
  vsnprintf(buf, sizeof(buf), error, ap);
  fprintf(stderr, "%s%slinker error: %s\n", filename, file == NULL ? "" : ": ",
          buf);
  if (file != NULL && file->linker != NULL) {
    file->linker->num_errors++;
  }
}

void LinkerWarning(ObjectFile* file, const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLinkerWarning(file, warn, error, ap);
  va_end(ap);

}

void VLinkerWarning(ObjectFile* file, const char* warn, const char* error, va_list ap) {
  const char* filename = "";
  if (file != NULL) {
    filename = file->elf_file->filename.value;
  }
  char buf[1024];
  vsnprintf(buf, sizeof(buf), error, ap);
  fprintf(stderr, "%s: linker warning: %s: %s\n", filename, warn, buf);

}


void LinkerInitConfigLayout(Linker* linker,
                            const char* config_file,
                            const char* layout_type_name) {
  ConfigParserInit(&linker->config_parser, config_file);
  bool ok = ConfigParserParse(&linker->config_parser);
  if (!ok) {
    fprintf(stderr, "Failed to parse linker config file %s\n", config_file);
    exit(1);
  }
  // Find the layout in the config.  This is done by machine id and layout
  // type name.
  ConfigNode* layouts = ConfigObjectFind(linker->config_parser.root, "layout");
  if (layouts == NULL) {
    fprintf(stderr, "There are no layouts in config file %s\n", config_file);
    exit(1);
  }
  ConfigNodeVectorize(layouts);
  
  ConfigNode* layout_found = NULL;
  for (size_t i = 0; i < layouts->value.vector_value.length; i++) {
    ConfigNode* layout = layouts->value.vector_value.value.p[i];
    ConfigNode* machine = ConfigObjectFind(layout->value.object_value, "machine");
    if (machine != NULL) {
      if (machine->value.int_value == linker->elf_machine_type) {
        ConfigNode* type = ConfigObjectFind(layout->value.object_value, "type");
        if (type != NULL) {
          if (StringEqual(&type->value.string_value, layout_type_name)) {
            layout_found = layout;
            break;
          }
        }
      }
    }
  }
  if (layout_found == NULL) {
    fprintf(stderr, "Cannot find layout type %s in config file %s\n", layout_type_name, config_file);
    exit(1);
  }
  LinkerConfigInit(&linker->config, layout_found->value.object_value);
  if (linker->config.errors != 0) {
    fprintf(stderr, "Exiting due to config errors\n");
    exit(1);
  }
}


void LinkerInit(Linker* linker) {
  StringInit(&linker->output_filename, "a.out");
  VectorInit(&linker->files);
  VectorInit(&linker->architectures);
  HashTableInit(&linker->global_symbol_table, "global-symbol-table", 1009,
                LinkerSymbolHash, LinkerSymbolInsertInHashTable, LinkerSymbolFindInHashTable);
  VectorInit(&linker->section_groups);
  VectorInit(&linker->library_search_path);
  VectorInit(&linker->static_libraries);
  VectorInit(&linker->dynamic_libraries);
  VectorInit(&linker->needed_libraries);
  VectorInit(&linker->rpath);
  StringInit(&linker->interpreter, "/lib/ld.so");
  StringInit(&linker->entry_symbol, "_start");
  
  // Create the initial library search path.
  VectorAppend(&linker->library_search_path, NewString("/usr/lib"));
  VectorAppend(&linker->library_search_path, NewString("/lib"));
  
  linker->elf_machine_type = 0;
  linker->elf_flags = 0;
  linker->building_dso = false;
  linker->so_name = -1;
  linker->fully_static = false;
  linker->origin = 0;
  linker->num_errors = 0;

  linker->print_relocations = false;
  linker->print_symbol_tables = false;
  linker->print_sections = false;

  // Initialize and add the architectures.
  VectorAppend(&linker->architectures, NewPCodeLinkerArchitecture());
  VectorAppend(&linker->architectures, NewRISCVLinkerArchitecture());
  VectorAppend(&linker->architectures, New6502LinkerArchitecture());
  VectorAppend(&linker->architectures, NewAARCH64LinkerArchitecture());

  // Add the contents of LD_LIBRARY_PATH to the library search path.
  char* ld_library_path = getenv("LD_LIBRARY_PATH");
  if (ld_library_path != NULL) {
    char* start = ld_library_path;
    while (*start != '\0') {
      char* p = start;
      while (*p != ':' && *p != '\0') {
        p++;
      }
      String* dir = NewStringWithLength(start, p - start);
      VectorAppend(&linker->library_search_path, dir);
      start = p;
      if (*start == ':') {
        start++;
      }
    }
  }
}

void LinkerInitDynamic(Linker* linker) {
  linker->dynamic_linker = NewDynamicLinker(linker);
}


void LinkerDestruct(Linker* linker) {
  StringDestruct(&linker->output_filename);
  StringDestruct(&linker->entry_symbol);
  VectorDestructWithContents(&linker->files, (VectorElementDestructor)ObjectFileDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->architectures, NULL, /*free_element=*/true);
  LinkerClearSymbolTable(&linker->global_symbol_table);
  HashTableDestruct(&linker->global_symbol_table);
  VectorDestructWithContents(&linker->section_groups, (VectorElementDestructor)SectionGroupDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->static_libraries, (VectorElementDestructor)ARArchiveDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->dynamic_libraries, (VectorElementDestructor)LoadedDynamicLibraryDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->library_search_path, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->needed_libraries, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->rpath, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
}

void LinkerInitArchitecture(Linker* linker) {
  for (size_t i = 0; i < linker->architectures.length; i++) {
    LinkerArchitecture* arch = linker->architectures.value.p[i];
    if (arch->machine_type == linker->elf_machine_type) {
      linker->arch = arch;
      arch->check_options(linker);
      return;
    }
  }
  fprintf(stderr, "Unsupported linker architecture %d\n",
          linker->elf_machine_type);
  exit(1);
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
    String* dir = linker->library_search_path.value.p[i];
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
  String libname;
  StringInit(&libname, name);
  LoadedDynamicLibrary* lib = DynamicLoaderFindLibrary(
                                                       &linker->dynamic_linker->loaded_dynamic_libraries,
                                                       &libname);
  if (lib == NULL) {
    lib = NewLoadedDynamicLibrary(name, NULL);
    DynamicLibraryRegistryInsert(&linker->dynamic_linker->loaded_dynamic_libraries, lib);

    bool ok = LoadedDynamicLibraryLoad(lib, &linker->dynamic_linker->loaded_dynamic_libraries,
                             &linker->library_search_path, 0, NULL);
    if (!ok) {
      StringDestruct(&libname);
      return;
    }
    VectorAppend(&linker->dynamic_libraries, lib);
  }
  StringDestruct(&libname);
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
    *archive = linker->static_libraries.value.p[i];
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

SectionGroup* NewSectionGroup(const String* name, int32_t type,
                              int64_t flags, int64_t alignment) {
  SectionGroup* group = malloc(sizeof(SectionGroup));
  StringInit(&group->name, name->value);
  VectorInit(&group->components);
  group->type = type;
  group->flags = flags;
  group->alignment = alignment;
  group->segment = NULL;
  group->address = 0;
  group->region = NULL;
  return group;
}

void SectionGroupDestruct(SectionGroup* group) {
  StringDestruct(&group->name);
  VectorDestructWithContents(&group->components,
                             (VectorElementDestructor)GroupedSectionDestruct, /*free_element=*/true);
}

void SectionGroupDelete(SectionGroup* group) {
  SectionGroupDestruct(group);
  free(group);
}

// Grouped sections, new or existing sections in a SectionGroup.
GroupedSection* NewExistingGroupedSection(ELFReaderSection* section) {
  GroupedSection* sect = malloc(sizeof(GroupedSection));
  sect->source = kGroupedSectionExisting;
  sect->section.existing = section;
  return sect;
}

GroupedSection* NewGroupedSection(ELFWriterSection* section) {
  GroupedSection* sect = malloc(sizeof(GroupedSection));
  sect->source = kGroupedSectionNew;
  sect->section.new = section;
  return sect;
}

GroupedSection* NewGroupedSectionPadding(ELFWriterSectionContents* contents) {
  GroupedSection* sect = malloc(sizeof(GroupedSection));
  sect->source = kGroupedSectionPadding;
  sect->section.padding = contents;
  return sect;
}

void GroupedSectionDestruct(GroupedSection* g) {
  switch (g->source) {
    case kGroupedSectionNew:
      ELFWriterSectionDelete(g->section.new);
      break;
    case kGroupedSectionExisting:
      break;
    case kGroupedSectionPadding:
      break;
  }
}

void SegmentMemoryRegionInit(SegmentMemoryRegion* region, ConfigRegion* config) {
  StringInit(&region->name, config->name.value);
  region->start = config->start_addr;
  region->config_end =  config->size == 0 ? 0 : region->start + config->size;
  region->actual_end = 0;
  region->next = region->start;
  region->falign = config->falign;
  VectorInit(&region->sections);
  for (size_t i = 0; i < config->sections.length; i++) {
    VectorAppend(&region->sections, NewString(config->sections.value.p[i]));
  }
}

SegmentMemoryRegion* NewSegmentMemoryRegion(ConfigRegion* config) {
  SegmentMemoryRegion* r = malloc(sizeof(SegmentMemoryRegion));
  SegmentMemoryRegionInit(r, config);
  return r;
}

void SegmentMemoryRegionDestruct(SegmentMemoryRegion* region) {
  StringDestruct(&region->name);
  VectorDestructWithContents(&region->sections, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
}

void SegmentMemoryRegionDelete(SegmentMemoryRegion* region) {
  SegmentMemoryRegionDestruct(region);
  free(region);
}

SegmentMemoryRegion* NewInternalSegmentMemoryRegion(void) {
  SegmentMemoryRegion* r = malloc(sizeof(SegmentMemoryRegion));
  VectorInit(&r->sections);
  r->start = 0;
  r->actual_end = r->config_end = 0;
  r->next = 0;
  r->falign = false;
  return r;
}

void SegmentInit(Segment* segment, ConfigSegment* config) {
  segment->config = config;
  VectorInit(&segment->sections);
  VectorInit(&segment->regions);
  
  if (config != NULL) {
    // Build regions.
    for (size_t i = 0; i < config->regions.length; i++) {
      ConfigRegion* rconfig = config->regions.value.p[i];
      SegmentMemoryRegion* region = NewSegmentMemoryRegion(rconfig);
      VectorAppend(&segment->regions, region);
    }
  }
}

void SegmentDestruct(Segment* segment) {
  VectorDestruct(&segment->sections);
  VectorDestructWithContents(&segment->regions, (VectorElementDestructor)SegmentMemoryRegionDestruct, /*free_element=*/true);
}

SegmentMemoryRegion* SegmentDefaultRegion(Segment* segment) {
  if (segment->regions.length == 1) {
    return segment->regions.value.p[0];
  }
  SegmentMemoryRegion* r = NewInternalSegmentMemoryRegion();
  VectorAppend(&segment->regions, r);
  return r;
}

// Assign a memory region to a segment group.
static void AssignGroupRegion(Segment* segment, SectionGroup* group) {
  int region_index = -1;
   String* section_name = &group->name;
   for (size_t i = 0; region_index == -1 && i < segment->regions.length; i++) {
     SegmentMemoryRegion* region = segment->regions.value.p[i];
     for (size_t j = 0; j < region->sections.length; j++) {
       if (StringEqualString(region->sections.value.p[j], section_name)) {
         region_index = (int)i;
         break;
       }
     }
   }
  if (region_index == -1) {
    // No region for this section.  Ignore.
    return;
  }
  SegmentMemoryRegion* region = segment->regions.value.p[region_index];
  group->region = region;
}

static uint64_t RegionAllocateAddress(Linker* linker, SectionGroup* group,
                                uint64_t size, uint64_t last_segment_end) {
  // Make sure we have enough space in the region.
  SegmentMemoryRegion* region = group->region;
  assert(region != NULL);
  if (region->config_end != 0) {
    uint64_t next = region->next + size;
    if (next > region->config_end) {
      LinkerError(NULL, "Cannot add contents of section '%s' with size %zd to memory region '%s'",
                  group->name.value, (size_t)size, region->name.value);
      return 0;
    }
  }
  if (region->next == 0) {
    region->next = last_segment_end;
  }
  uint64_t addr = region->next;
  region->next += size;
  return addr;
}

uint64_t SegmentEndAddress(Segment* segment) {
  for (ssize_t i = segment->regions.length-1; i >= 0; i--) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    if (region->start != 0) {
      if (region->config_end != 0) {
        return region->config_end;
      }
      return region->next;
    } else if (region->next != 0) {
      return region->next;
    }
  }
 
  return 0;
}


void SetSegmentStartAddress(Segment* segment, uint64_t addr) {
  for (size_t i = 0; i < segment->regions.length; i++) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    if (region->start == 0) {
      region->start = addr;
      return;
    }
  }
}

void SetSegmentEndAddress(Segment* segment, uint64_t addr) {
  for (ssize_t i = segment->regions.length-1; i >= 0; i--) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    if (region->start != 0) {
      region->actual_end = addr;
      return;
    }
  }
}

uint64_t RegionNextAddress(SectionGroup* group) {
  SegmentMemoryRegion* region = group->region;
  assert(region != NULL);
  return region->next;
}

uint64_t RegionPadding(SectionGroup* group) {
  SegmentMemoryRegion* region = group->region;
  assert(region != NULL);
  if (region->config_end != 0) {
    return region->config_end - region->next;
  }
  return 0;
}

// Data for passing to map traverse function to build the section groups.
struct SectionGroupingData {
  Linker* linker;
  int32_t section_type;
};

// Build the symbol tables.  There is probably only one in the file
// but we should find them all just in case.  Each symbol table section
// says where its string table is in the 'link' field in the header.
static void ReadSymbolTables(Linker* linker, ELFReaderFile* elf_file,
                             ObjectFile* file) {
  Vector symbol_tables;
  VectorInit(&symbol_tables);
  ELFReaderFileFindSectionsByType(elf_file, SHT(symtab), &symbol_tables);
  
  for (size_t sect = 0; sect < symbol_tables.length; sect++) {
    ELFReaderSection* symtab = symbol_tables.value.p[sect];
    if (symtab->header->link >= elf_file->sections.length) {
      LinkerError(file, "Corrupt symbol table link value");
      continue;
    }
    ELFReaderSection* strtab = elf_file->sections.value.p[symtab->header->link];
    size_t num_symbols = symtab->header->size / symtab->header->entsize;
    const char* symbol_addr = (const char*)elf_file->header + symtab->header->offset;
    
    // Now read the symbols and add them to the symbol tables in the file.
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol* elf_sym = (ELFSymbol*)symbol_addr;
      LinkerReadSymbol(linker, file, elf_file, strtab, elf_sym);
      symbol_addr += symtab->header->entsize;
    }
  }
  VectorDestruct(&symbol_tables);
}

static void ReadRelocations(Linker* linker, ELFReaderFile* elf_file,
                             ObjectFile* file) {
  // Build the relocation tables.
  Vector relocation_sections;
  VectorInit(&relocation_sections);
  
  // Find all relocation sections.  Finds both REL and RELA sections.
  ELFReaderFileFindSectionsByType(elf_file, SHT(rela), &relocation_sections);
  ELFReaderFileFindSectionsByType(elf_file, SHT(rel), &relocation_sections);
  
  // Process all relocation sections.
  for (size_t i = 0; i < relocation_sections.length; i++) {
    ELFReaderSection* reloc_section = relocation_sections.value.p[i];
    int32_t symtab_section_index = reloc_section->header->link;
    if (symtab_section_index >= elf_file->sections.length) {
      LinkerError(file, "Corrupt relocation symbol table section index");
      continue;
    }
    ELFReaderSection* symtab = elf_file->sections.value.p[symtab_section_index];
    
    if (symtab->header->link >= elf_file->sections.length) {
      LinkerError(file, "Corrupt relocation symbol section index");
      continue;
    }
    ELFReaderSection* strtab = elf_file->sections.value.p[symtab->header->link];
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

static void ReadELFContents(Linker* linker, ELFReaderFile* elf_file,
                            ObjectFile* file) {
  // Build the sections_by_name map in the ObjectFile.  This is a map
  // whose key is the section name (String*) and the value is and ELFReaderSection*.
  // Also build the sections_by_type map.
  for (size_t i = 0; i < elf_file->sections.length; i++) {
    ELFReaderSection* section = elf_file->sections.value.p[i];
    MapKeyValue kv;
    kv.key.p = &section->name;
    kv.value.p = section;
    MapInsert(&file->sections_by_name, kv);
    
    Vector* type_list = MapFindInt64Key(&file->sections_by_type, section->header->type);
    if (type_list == NULL) {
      type_list = NewVector();
      MapKeyValue kv;
      kv.key.w = section->header->type;
      kv.value.p = type_list;
      MapInsert(&file->sections_by_type, kv);
    }
    VectorAppend(type_list, section);
  }
  
  // Read the symbol tables.
  ReadSymbolTables(linker, elf_file, file);
  
  // Read the relocations.
  ReadRelocations(linker, elf_file, file);
}

static bool CheckMachineType(Linker* linker, ObjectFile* file) {
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
  ObjectFile* file = NewObjectFile(elf_file, linker, filename->value);
  VectorAppend(&linker->files, file);
  bool ok = ELFReaderFileRead(elf_file, 0, 0);
  if (!ok) {
    return false;
  }
  if (!CheckMachineType(linker, file)) {
    return false;
  }

  ReadELFContents(linker, elf_file, file);
  return true;
}

bool LinkerReadObjectFileFromArchive(Linker* linker, ARArchive* archive,
                                     ARFile* ar_file) {
  ELFReaderFile* elf_file = NewELFReaderFile(&archive->filename);
  ObjectFile* file = NewObjectFile(elf_file, linker, ar_file->filename.value);
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

// Check if a symbol table bucket contains undefined symbola
// and if so, look for them in the libraries and if found
// link in the library file defining them.
static void ResolveUndefined(void* entry, void* data) {
  Vector* bucket = entry;
  Linker* linker = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value.p[i];
    if (!symbol->defined) {
      ARArchive* archive;
      ARFile* file;
      bool found = LinkerFindSymbolInStaticLibraries(linker, symbol->name.value,
                                               &archive, &file);
      if (found) {
        LinkerReadObjectFileFromArchive(linker, archive, file);
        continue;
      }
      
      // Look in dynamic libraries.  If it is found, mark it as
      // resolved externally but don't link in the library.  It will
      // be done at runtime.
      const ELFSymbol* dynamic_sym;
      LoadedDynamicLibrary* found_lib;
      bool ok = DynamicLoaderFindSymbol(
                          &linker->dynamic_linker->loaded_dynamic_libraries,
                          symbol->name.value,
                                        &dynamic_sym,
                                        &found_lib);
      if (ok) {
        symbol->defined = true;
      }
    }
  }
}

static void ResolveEntrySymbol(Linker* linker) {
  String* entry_name = &linker->entry_symbol;
  ARArchive* archive;
   ARFile* file;
   bool found = LinkerFindSymbolInStaticLibraries(linker, entry_name->value,
                                            &archive, &file);
   if (found) {
     LinkerReadObjectFileFromArchive(linker, archive, file);
     return;
   }
   
   // Look in dynamic libraries.  If it is found, mark it as
   // resolved externally but don't link in the library.  It will
   // be done at runtime.
   const ELFSymbol* dynamic_sym;
   LoadedDynamicLibrary* found_lib;
   DynamicLoaderFindSymbol(
                       &linker->dynamic_linker->loaded_dynamic_libraries,
                       entry_name->value,
                                     &dynamic_sym,
                                     &found_lib);

}

// Resolve all undefined symbols in the libraries.  Loop until we get no more
// symbols added to the global table from loaded libraries.
static void ResolveUndefinedSymbols(Linker* linker) {
  ResolveEntrySymbol(linker);
  size_t old_num_symbols, new_num_symbols;
  do {
    old_num_symbols = linker->global_symbol_table.object_count;
    HashTableTraverse(&linker->global_symbol_table, ResolveUndefined, linker);
    new_num_symbols = linker->global_symbol_table.object_count;
  } while (old_num_symbols != new_num_symbols);
}

// This is a map traversal function that is called for each entry in a
// mapping of section name to a vector of ELFReaderSection pointers, all
// of which share a common name and type.  We build a SectionGroup struct
// that contains the same ELFReaderSection pointers from the map entry held
// inside GroupedSection structs.
static void BuildSectionGroup(MapKeyValue* kv, void* data) {
  const String* name = kv->key.p;
  Vector* sections = kv->value.p;
  Linker* linker = ((struct SectionGroupingData*)data)->linker;
  int32_t section_type = ((struct SectionGroupingData*)data)->section_type;

  // Take the flags and alignment from the first section.
  ELFReaderSection* first_section = sections->value.p[0];
  int64_t section_flags = first_section->header->flags;
  int64_t section_alignment = first_section->header->addralign;

  SectionGroup* group = NewSectionGroup(name, section_type, section_flags,
                                        section_alignment);
  VectorAppend(&linker->section_groups, group);

  // Append all sections in the vector of ELFReaderSections to
  // the components of the group.  Each element of the components
  // vector is a GroupedSection object with source kGroupedSectionExisting.
  for (size_t i = 0; i < sections->length; i++) {
    ELFReaderSection* s = sections->value.p[i];
    GroupedSection* gsection = NewExistingGroupedSection(s);
    VectorAppend(&group->components, gsection);
  }
}

// Print a section map key/value pair.
static void PrintSectionMapKV(MapKeyValue* kv, void* data) {
  const String* name = kv->key.p;
  const Vector* sections = kv->value.p;
  printf("Name: %s\n", name->value);
  for (size_t i = 0; i < sections->length; i++) {
    ELFReaderSection* section = sections->value.p[i];
    printf("  [%zd]: %s %p @%" PRIx64 "\n", i, section->name.value, section->contents, section->address);
  }
}

static void PrintSectionMap(Map* map) {
  MapTraverse(map, PrintSectionMapKV, NULL);
}

// Group all sections with the given type into a the section_groups
// vector in the Linker.
static void GroupSections(Linker* linker, int32_t section_type,
                          int32_t section_flags) {
  Map section_map;
  MapInitForStringKeys(&section_map);

  // Build a map of section name vs vectors of pointers to ELFReaderSections
  // with the type given.  A traversal will be in alphabetic order by
  // section name.
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];

    Vector* sections = MapFindInt64Key(&file->sections_by_type,
                               section_type);
    if (sections == NULL) {
      continue;
    }
    for (size_t j = 0; j < sections->length; j++) {
      ELFReaderSection* section = sections->value.p[j];
      if (section_flags != 0 && (section->header->flags & section_flags) == 0) {
        continue;
      }
      Vector* result_vec = MapFindPointerKey(&section_map, &section->name);
      if (result_vec == NULL) {
        result_vec = NewVector();
        MapKeyValue kv;
        kv.key.p = &section->name;
        kv.value.p = result_vec;
        MapInsert(&section_map, kv);
      }
      VectorAppend(result_vec, section);
    }
  }

  if (linker->print_sections) {
    PrintSectionMap(&section_map);
  }
  
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

  // We don't need this section map now that we have the section groups.
  MapDestruct(&section_map);
}

static bool SegmentContainsSection(Segment* segment, String* section_name) {
  for (size_t i = 0; i < segment->config->regions.length; i++) {
    ConfigRegion* region = segment->config->regions.value.p[i];
    for (size_t j = 0; j < region->sections.length; j++) {
      if (StringEqualString(region->sections.value.p[j], section_name)) {
        return true;
      }
    }
  }
  return false;
}

// Now we have the sections grouped we can assign each section to its
// appropriate segment.  TLS are always in the tls_segment but other sections
// need to be assigned to a segment in the config file.
static void AssignSectionGroupsToSegments(Linker* linker) {
  for (size_t i = 0; i < linker->section_groups.length; i++) {
    SectionGroup* group = linker->section_groups.value.p[i];
    Segment* segment = NULL;
    if ((group->flags & SHF(tls)) != 0) {
      segment = &linker->tls_segment;
    } else if (SegmentContainsSection(&linker->code_segment, &group->name)) {
      segment = &linker->code_segment;
    } else if (SegmentContainsSection(&linker->data_segment, &group->name)) {
      segment = &linker->data_segment;
    }
    if (segment == NULL) {
      LinkerError(NULL, "Cannot find segment for section %s", group->name.value);
      return;
    }
    group->segment = segment;
    VectorAppend(&segment->sections, group);
  }
}

static LinkerSymbol* InventSymbol(Linker* linker, const char* name, int size, uint64_t address) {
  LinkerSymbol* sym = LinkerFindSymbol(&linker->global_symbol_table, name);
  if (sym == NULL) {
    sym = LinkerInventSymbol(linker, name, size);
  }
  sym->defined = true;
  sym->size = size;
  sym->address = address;
  return sym;
}

static int CompareGroupRegion(const void* a, const void* b) {
  const SectionGroup* g1 = *(const SectionGroup**)a;
  const SectionGroup* g2 = *(const SectionGroup**)b;
  return (int)(g1->region->start - g2->region->start);
}

static uint64_t AlignedStartAddress(SegmentMemoryRegion* region, uint64_t file_offset) {
  uint64_t start_addr = region->start;
  if (region->falign) {
    start_addr += file_offset;
    region->next += file_offset;
  }
  return start_addr;
}

// Assign addresses to all the sections held within the segment.
// The starting address is passed.
static void AssignSegmentSectionAddresses(Linker* linker, Segment* segment, uint64_t last_segment_end, uint64_t file_offset) {
  for (size_t i = 0; i < segment->sections.length; i++) {
    SectionGroup* group = segment->sections.value.p[i];
    AssignGroupRegion(segment, group);
  }
  // Sort regions in address order.
  VectorSortPointers(&segment->sections, CompareGroupRegion);
  uint64_t addr = last_segment_end;
  if (addr != 0) {
    // If we know the start address, set it now.  Otherwise we delay until we
    // know it.
    // See if we have a start address in the group's regions.  If so
    // we need to use that.
    uint64_t start_addr = 0;
    for (size_t i = 0; i < segment->sections.length; i++) {
      SectionGroup* group = segment->sections.value.p[i];
      if (group->region != NULL && group->region->start != 0) {
        start_addr = AlignedStartAddress(group->region, file_offset);
        break;
      }
    }
    // 6502 has the start_addr as zero here because the data segment is
    // placed immediately after the code segment.
    // TODO: does this break other configs?
    if (start_addr != 0) {
      SetSegmentStartAddress(segment, start_addr);
      addr = start_addr;
    }
  }
  
  int64_t offset = 0;
  for (size_t i = 0; i < segment->sections.length; i++) {
    SectionGroup* group = segment->sections.value.p[i];
    if (group->region == NULL) {
      // No region for this group, it's not in the output.
      continue;
    }
    if (addr == 0) {
      uint64_t start_addr = AlignedStartAddress(group->region, file_offset);
      SetSegmentStartAddress(segment, start_addr);
      addr = start_addr;
    }
    group->address = RegionNextAddress(group);
    if (group->address == 0) {
      group->address = last_segment_end;
    }
    // Concatenate all the component sections, assigning
    // consecutive addresses.
    for (size_t j = 0; j < group->components.length; j++) {
      GroupedSection* gsect = group->components.value.p[j];
      switch (gsect->source) {
        case kGroupedSectionExisting: {
          ELFReaderSection* section = gsect->section.existing;
          section->address = RegionAllocateAddress(linker,
                                                    group,
                                                    section->header->size,
                                                    addr);
          section->offset = offset;
          offset += section->header->size;
          addr += section->header->size;
          break;
        }
        case kGroupedSectionNew: {
          ELFWriterSection* section = gsect->section.new;
          section->address = RegionAllocateAddress(linker,
                                                    group,
                                                    section->contents->data.buffered.length,
                                                    addr);
          addr += section->contents->size;
          break;
        }
        case kGroupedSectionPadding:
          abort();      // Can't have any of these here.
          break;
        
      }
    }
    uint64_t padding = RegionPadding(group);
    if (padding == 0) {
      continue;
    }
    
    // Need some padding for this output section.  Add a padding section.
    ELFWriterSectionContents* pad = NewELFWriterSectionContents(kSectionContentsPad);
    pad->size = padding;
    addr += padding;
    GroupedSection* pad_group = NewGroupedSectionPadding(pad);
    VectorAppend(&group->components, pad_group);
 }
  
  SetSegmentEndAddress(segment, addr);
  
  // Now we need to order the groups by region and thus address.
  // VectorSortPointers(&segment->sections, CompareGroupRegion);
}

static ConfigSegment* FakeConfigSegment() {
  ConfigSegment* s = malloc(sizeof(ConfigSegment));
  VectorInit(&s->regions);
  return s;
}

// Link all files passed to the linker together.  This gathers the sections
// with the same names into the same place and assigns addresses to the
// sections and symbols.
void LinkerLinkAllFiles(Linker* linker) {
  // Create the segments from the config.
  bool code_init_done = false;
  bool data_init_done = false;

  for (size_t i = 0; i < linker->config.segments.length; i++) {
    ConfigSegment* seg = linker->config.segments.value.p[i];
    switch (seg->type) {
      case kConfigSegmentTypeText:
        SegmentInit(&linker->code_segment, seg);
        code_init_done = true;
        break;
      case kConfigSegmentTypeData:
        SegmentInit(&linker->data_segment, seg);
        data_init_done = true;
         break;
      case kConfigSegmentTypeDynamic:
        if (!linker->fully_static) {
          SegmentInit(&linker->dynamic_segment, seg);
        }
        break;
      case kConfigSegmentTypeInterp:
        if (!linker->fully_static && !linker->building_dso) {
           SegmentInit(&linker->interpreter_segment, seg);
         }
        break;
      default:
        break;
    }
  }
  SegmentInit(&linker->tls_segment, FakeConfigSegment());
  if (!code_init_done) {
    SegmentInit(&linker->code_segment, FakeConfigSegment());
  }
  if (!data_init_done) {
    SegmentInit(&linker->data_segment, FakeConfigSegment());
  }
  // Resolve all undefined symbols in libraries.
  ResolveUndefinedSymbols(linker);
  
  // Find all PROGBITS sections and group by name.  These are sections
  // that have data associated with them in the ELF file.  This also
  // adds the grouped sections to the appropriate segment (code, data or tls).
  GroupSections(linker, SHT(progbits), 0);
  
  // Group the TLS nobits sections.
  GroupSections(linker, SHT(nobits), SHF(tls));

  // Assign all section groups to their appropriate segments.
  AssignSectionGroupsToSegments(linker);
  
  // Create all the loadable sections for a dynamic library.  We need
  // to do this now so that the addresses of the sections can be calculated.
  // However, not all the information is available at this point so we
  // put placeholders in the section contents.
  if (!linker->fully_static) {
    DynamicLinkerCreateDynamicLinkerGroups(linker);
  }
  
  // Assign addresses to all sections.
 
  int num_program_headers = 5;
  int num_sections = 18;
  
  uint64_t text_file_offset = sizeof(ELFHeader) + sizeof(ELFProgramHeader) * num_program_headers +
  sizeof(ELFSectionHeader) * num_sections;
  
  // Assign code segment addresses.
  AssignSegmentSectionAddresses(linker, &linker->code_segment, 0, text_file_offset);

  uint64_t end_of_segments =  SegmentEndAddress(&linker->code_segment);
  if (!linker->fully_static) {
    // Assign addresses to the dynamic section.
    AssignSegmentSectionAddresses(linker, &linker->dynamic_segment, end_of_segments, 0);
  }
  
  if (!linker->fully_static && !linker->building_dso) {
    // Assign addresses to the interpreter section.
    end_of_segments = SegmentEndAddress(&linker->dynamic_segment);
    AssignSegmentSectionAddresses(linker, &linker->interpreter_segment, end_of_segments, 0);
  }
  
  // Define the '_etext' symbol for the last assigned address.
  InventSymbol(linker, "_etext", 8, end_of_segments);
  uint64_t addr = end_of_segments;

  // Assign addresses to sections in the data segment.  This must be
  // last since it also needs to contain the .bss section.
  uint64_t data_file_offset = text_file_offset ;

  AssignSegmentSectionAddresses(linker, &linker->data_segment, end_of_segments, data_file_offset);

  // The TLS segment starts at address 0 and doesn't increment the current
  // address.
  AssignSegmentSectionAddresses(linker, &linker->tls_segment, SegmentEndAddress(&linker->data_segment), 0);
 
  // Define the '_edata' symbol for the last assigned address.
  InventSymbol(linker, "_edata", 8, SegmentEndAddress(&linker->data_segment));

  // Now that we know the addresses of the sections we can work
  // out the values of the symbols within those sections.
  LinkerAssignSymbolAddresses(linker);
  
  // Assign section symbol addresses.
  LinkerAssignSectionSymbolAddresses(linker);
  
  // The .bss (nobits) address is just after all the other sections.
  linker->nobits_address = SegmentEndAddress(&linker->data_segment);
  if (linker->nobits_address != 0) {
  
    addr = linker->nobits_address;
    LinkerAssignCommonSymbolAddresses(linker, &addr);
    linker->nobit_size = addr - linker->nobits_address;
    LinkerAssignBSSSymbolAddresses(linker);
  } else {
    linker->nobit_size = 0;
  }

  // Define the '_end' symbol for the last assigned address.
  InventSymbol(linker, "_end", 8, addr);
  
  if (!linker->fully_static) {
    // Define the dynamic linker symbols.  This includes
    // _GLOBAL_OFFSET_TABLE_ and _DYNAMIC_.
    DynamicLinkerDefineSymbols(linker);
    
    // Fixup the GOT and PLT now that we have all the addresses.
    DynamicLinkerFixupGOT(linker);
    DynamicLinkerFixupPLT(linker);
  }
  
  if (linker->print_symbol_tables) {
    LinkerPrintSymbolTables(linker);
  }
  
  // We have all the values of the symbols, apply those values to
  // all the relocations in the files.
  LinkerApplyAllRelocations(linker);

  if (!linker->building_dso) {
    // Check if we have any undefined symbols and report errors if found.
    LinkerCheckForUndefinedSymbols(linker);
  }
}

// Build an output section from a group of sections.  Each component of the
// group is a GroupedSection that can come from an existing file or can
// be generated by the linker.
static void BuildOutputSection(ELFWriterFile* elf, Segment* segment,
                                            SectionGroup* group) {
  if (group->region == NULL) {
    return;
  }
  ELFWriterSectionContents* contents = NewELFWriterSectionContents(kSectionContentsMulti);
  ELFWriterSection* section = ELFWriterAddSection(elf, &group->name,
                      group->type,
                      group->flags,
                      group->alignment,
                      contents, group->address);
  for (size_t i = 0; i < group->components.length; i++) {
    GroupedSection* gsect = group->components.value.p[i];
    ELFWriterSectionContents* part_contents;
    switch (gsect->source) {
      case kGroupedSectionExisting: {
        ELFReaderSection* part = gsect->section.existing;
        if (part->header->type == SHT(progbits)) {
          part_contents = NewELFWriterSectionContents(kSectionContentsRaw);
          part_contents->size = part->header->size;
          part_contents->data.raw = part->contents;
        } else {
          part_contents = NewELFWriterSectionContents(kSectionContentsNobits);
          part_contents->size = part->header->size;
        }
        break;
      }
      case kGroupedSectionNew: {
        part_contents = gsect->section.new->contents;
        // Propagate entry size from component section if it is set.
        ELF_Xword entsize = gsect->section.new->header.entsize;
        if (entsize != 0) {
          section->header.entsize = entsize;
        }
        // Propagate user data if it is set.
        if (gsect->section.new->user_data != NULL) {
          section->user_data = gsect->section.new->user_data;
        }
        break;
      }
      case kGroupedSectionPadding:
        part_contents = gsect->section.padding;
        break;
    }

    VectorAppend(&contents->data.multi, part_contents);
  }
}

// Find the buffer containing the .dynsym section contents.  This
// is in an output section which means its contents is a vector
// of ELFWriterSectionContents pointers. There will be only
// one of them and it will be a buffered one.
static Buffer* FindDynamicSymbolTableBuffer(ELFWriterFile* elf) {
  ELFWriterSection* section = ELFWriterFindSection(elf, ".dynsym");
  assert(section != NULL);
  ELFWriterSectionContents* contents = section->contents->data.multi.value.p[0];
  return &contents->data.buffered;
}

static void AssignGroupSectionIndexes(SectionGroup* group, int32_t* index_ptr) {
  if (group->region == NULL) {
    return;
  }
  for (size_t i = 0; i < group->components.length; i++) {
    GroupedSection* gsect = group->components.value.p[i];
    switch (gsect->source) {
      case kGroupedSectionExisting: {
        ELFReaderSection* section = gsect->section.existing;
        section->output_section_index = *index_ptr;
        break;
      case kGroupedSectionNew:
        gsect->section.new->index = *index_ptr;
        break;
      case kGroupedSectionPadding:
        break;
      }
        
    }
  }
  // Move the index on.
  (*index_ptr)++;
}

// Add a bucket of symbols to the output ELF file.
static void AddSymbolListToOutput(void* entry, void* data) {
  Vector* bucket = entry;
  ELFWriterFile* elf = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value.p[i];
    int32_t type = ELF_ST_TYPE(sym->header->info);
    int32_t binding = ELF_ST_BIND(sym->header->info);
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
                       sym->address, &sym->index);
  }
}

static void BuildSections(Linker* linker, ELFWriterFile* elf) {
  // Add NULL section.
  ELFWriterSectionContents* null_contents = NewELFWriterSectionContents(kSectionContentsRaw);
  ELFWriterAddSection(elf, NULL,
                      SHT(null), 0, 8, null_contents, 0);
  
  // Build output sections in code segment.
  for (size_t i = 0; i < linker->code_segment.sections.length; i++) {
    BuildOutputSection(elf, &linker->code_segment, linker->code_segment.sections.value.p[i]);
  }
  
  if (!linker->fully_static) {
    for (size_t i = 0; i < linker->dynamic_segment.sections.length; i++) {
      BuildOutputSection(elf, &linker->dynamic_segment, linker->dynamic_segment.sections.value.p[i]);
    }
  }
  
  if (!linker->fully_static && !linker->building_dso) {
    for (size_t i = 0; i < linker->interpreter_segment.sections.length; i++) {
      BuildOutputSection(elf, &linker->interpreter_segment, linker->interpreter_segment.sections.value.p[i]);
    }
  }
  
  // Build output sections in data segment.
  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    BuildOutputSection(elf, &linker->data_segment, linker->data_segment.sections.value.p[i]);
  }
  
  // Build output sections in tls segment.
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    BuildOutputSection(elf, &linker->data_segment, linker->tls_segment.sections.value.p[i]);
  }

  // Add BSS section.
  ELFWriterSectionContents* bss_contents =
    NewELFWriterSectionContents(kSectionContentsNobits);
  bss_contents->size = linker->nobit_size;
  String bss_name;
  StringInit(&bss_name, ".bss");
  ELFWriterAddSection(elf, &bss_name,
                      SHT(nobits),
                      SHF(alloc) | SHF(write),
                      8,
                      bss_contents, linker->nobits_address);
  StringDestruct(&bss_name);
}

static void AssignSectionIndexes(Linker* linker, ELFWriterFile* elf) {
  int32_t section_index = 1;
  if (!linker->fully_static && !linker->building_dso) {
    for (size_t i = 0; i < linker->interpreter_segment.sections.length; i++) {
      AssignGroupSectionIndexes(linker->interpreter_segment.sections.value.p[i], &section_index);
    }
  }
  
  for (size_t i = 0; i < linker->code_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->code_segment.sections.value.p[i], &section_index);
  }
  
  if (!linker->fully_static) {
    for (size_t i = 0; i < linker->dynamic_segment.sections.length; i++) {
      AssignGroupSectionIndexes(linker->dynamic_segment.sections.value.p[i], &section_index);
    }
    
    // Fixup the dynamic symbol table now that we know the symbol addresses.
    DynamicLinkerFixupDynamicSymbolTable(FindDynamicSymbolTableBuffer(elf),
                                         (int32_t)elf->sections.length - 1);
    
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".dynsym", ".dynstr");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".rela.dyn", ".dynsym");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".rela.plt", ".dynsym");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldInfo, ".rela.plt", ".plt");
  }
  
  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->data_segment.sections.value.p[i], &section_index);
  }
  
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->tls_segment.sections.value.p[i], &section_index);
  }
}

// Now that we have all the sections assigned to their segments, insert
// the segments into the ELF file.
static void InsertSegments(Linker* linker, ELFWriterFile* elf) {
  ELFWriterSegment* code_segment = NewELFWriterSegment(PT(load),
                                                       PF(r) | PF(x),
                                                       linker->code_segment.config->alignment);
  ELFWriterSegment* data_segment = NewELFWriterSegment(PT(load),
                                                       PF(r) | PF(w),
                                                       linker->data_segment.config->alignment);
  // TODO: TLS alignment.
  ELFWriterSegment* tls_segment = NewELFWriterSegment(PT(tls),
                                                       PF(r),
                                                       linker->data_segment.config->alignment);
  ELFWriterSegment* dynamic_segment = NULL;
  ELFWriterSegment* interpreter_segment = NULL;
  if (!linker->fully_static) {
    dynamic_segment = NewELFWriterSegment(PT(dynamic), PF(r) | PF(w), 8);
  }
  if (!linker->fully_static && !linker->building_dso) {
    interpreter_segment = NewELFWriterSegment(PT(interp), PF(r), 1);
  }
  
  // We have added all output sections to the ELF file.  Now we can
  // add them all to the code and data segments in the ELF file.
  // NOTE: the first section is a NULL section so we don't count that.
  for (size_t i = 1; i < elf->sections.length; i++) {
    ELFWriterSection* section = elf->sections.value.p[i];
    if (section->header.type == SHT(dynamic)) {
      ELFWriterSegmentAddSection(dynamic_segment, section);
    } else if (StringEqual(&section->name, ".interp")) {
      ELFWriterSegmentAddSection(interpreter_segment, section);
    } else if ((section->header.flags & SHF(tls)) != 0) {
      ELFWriterSegmentAddSection(tls_segment, section);
    } else if ((section->header.flags & SHF(write)) == 0) {
      ELFWriterSegmentAddSection(code_segment, section);
    } else {
      ELFWriterSegmentAddSection(data_segment, section);
    }
  }
  
  // Append the segments to the ELF file.
  VectorAppend(&elf->segments, code_segment);
  VectorAppend(&elf->segments, data_segment);
  
  // If we have any TLS sections add the tls segment to the output
  // file.  Otherwise we can delete the empty segment.
  if (tls_segment->sections.length > 0) {
    VectorAppend(&elf->segments, tls_segment);
  } else {
    ELFWriterSegmentDelete(tls_segment);
  }
  
  if (!linker->fully_static) {
    VectorAppend(&elf->segments, dynamic_segment);
  }
  if (!linker->fully_static && !linker->building_dso) {
    VectorAppend(&elf->segments, interpreter_segment);
  }
}

static bool SetEntryAddress(Linker* linker, ELFWriterFile* elf) {
  if (linker->building_dso) {
    // Set entry to address of .text section.
    ELFWriterSection* text = ELFWriterFindSection(elf, ".text");
    if (text == NULL) {
      return false;
    }
    elf->header.entry = text->address;
  } else {
    // Find the entry point for the program.
    LinkerSymbol* entry = LinkerFindSymbol(&linker->global_symbol_table,
                                          linker->entry_symbol.value);
    if (entry == NULL) {
      LinkerError(NULL, "Cannot find entry symbol '%s'", linker->entry_symbol.value);
      return false;
    }
    elf->header.entry = entry->address;
  }
  return true;
}

// Write the output file.
bool LinkerWriteOutput(Linker* linker, FILE* output) {
  ELFWriterFile elf;

  ELFWriterFileInit(&elf,
                    linker->building_dso ? ET(dyn) : ET(exec),
                    linker->elf_machine_type,
                    linker->elf_flags,
                    linker->fully_static ? NULL :
                      DynamicLinkerFixupDynamicSectionContents,
                    true, true);
  
  // Build all output sections.
  BuildSections(linker, &elf);
  
  // Set the entry address in the ELF header.
  if (!SetEntryAddress(linker, &elf)) {
    ELFWriterFileDestruct(&elf);
    return false;
  }
  
  // Assign the section indexes for the output sections.
  AssignSectionIndexes(linker, &elf);

  // Add symbols to the ELF symbol table.
  HashTableTraverse(&linker->global_symbol_table, AddSymbolListToOutput, &elf);

  // Now thet we know all the symbol indexes we can create the dynamic
  // relocations.
  if (!linker->fully_static) {
    DynamicLinkerBuildDynamicRelocations(linker);
    DynamicLinkerBuildPLTRelocations(linker);
  }
  
  // Now build the code and data segments in the ELF output file.
  InsertSegments(linker, &elf);
  
  // Write the ELF file.
  ELFWriterFileWrite(&elf, output);
  
  // We're done with ELF file now.
  ELFWriterFileDestruct(&elf);
  return true;
}

