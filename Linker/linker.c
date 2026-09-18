//
//  linker.c
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker.h"
#include "linker_script.h"
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
#include "linker_gc.h"
#include "linker_stacktrace.h"
#include <sys/stat.h>
#include <limits.h>

#define LINKER_ARRAY_DEFAULT_PRIORITY 65535

// Supported architectures.
#include "linker_arch_pcode.h"
#include "linker_arch_riscv.h"
#include "linker_arch_6502.h"
#include "linker_arch_aarch64.h"
#include "linker_arch_arm.h"
#include "linker_arch_x86_64.h"
#include "linker_arch_x86.h"
#include "linker_arch_xtensa.h"
#include "linker_arch_bpf.h"

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
  bool ok;
  if (config_file != NULL) {
    ok = LinkerScriptParseFile(config_file, linker->elf_machine_type,
                               &linker->config);
    if (!ok) {
      fprintf(stderr, "Failed to parse linker script %s\n", config_file);
      exit(1);
    }
  } else {
    ok = LinkerScriptLoadBuiltin(linker->elf_machine_type,
                                 linker->ops->is_64_bit, layout_type_name,
                                 &linker->config);
    if (!ok) {
      fprintf(stderr, "Failed to load built-in linker script for layout %s\n",
              layout_type_name);
      exit(1);
    }
  }
  if (linker->config.errors != 0) {
    fprintf(stderr, "Exiting due to linker script errors\n");
    exit(1);
  }
  if (linker->config.entry_symbol.length > 0 &&
      StringEqual(&linker->entry_symbol, "_start")) {
    StringSet(&linker->entry_symbol, linker->config.entry_symbol.value);
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
  // Default to ELF64; overridden from the first object file read.
  linker->ops = ELFFormatOpsFor(true);
  linker->building_dso = false;
  linker->so_name = -1;
  linker->fully_static = false;
  linker->bind_now = false;
  linker->defer_program_init = false;
  linker->origin = 0;
  linker->stacktrace_info = NULL;
  linker->num_errors = 0;

  linker->gc_sections = false;
  linker->print_gc_sections = false;
  linker->print_relocations = false;
  linker->print_symbol_tables = false;
  linker->print_sections = false;
  LinkerConfigInitEmpty(&linker->config);

  // Initialize and add the architectures.
  VectorAppend(&linker->architectures, NewPCodeLinkerArchitecture());
  VectorAppend(&linker->architectures, NewRISCVLinkerArchitecture());
  VectorAppend(&linker->architectures, New6502LinkerArchitecture());
  VectorAppend(&linker->architectures, NewAARCH64LinkerArchitecture());
  VectorAppend(&linker->architectures, NewARMLinkerArchitecture());
  VectorAppend(&linker->architectures, NewX86_64LinkerArchitecture());
  VectorAppend(&linker->architectures, NewX86LinkerArchitecture());
  VectorAppend(&linker->architectures, NewXtensaLinkerArchitecture());
  VectorAppend(&linker->architectures, NewBPFLinkerArchitecture());

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
  LinkerStacktraceDestruct(linker);
  VectorDestructWithContents(&linker->section_groups, (VectorElementDestructor)SectionGroupDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->static_libraries, (VectorElementDestructor)ARArchiveDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->dynamic_libraries, (VectorElementDestructor)LoadedDynamicLibraryDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->library_search_path, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->needed_libraries, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  VectorDestructWithContents(&linker->rpath, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  LinkerConfigDestruct(&linker->config);
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
  region->trailing_align = config->trailing_align;
  VectorInit(&region->sections);
  for (size_t i = 0; i < config->sections.length; i++) {
    String* section = config->sections.value.p[i];
    VectorAppend(&region->sections, NewString(section->value));
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
  r->trailing_align = 0;
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
       String* pattern = region->sections.value.p[j];
       if (LinkerConfigPatternMatch(pattern->value, section_name->value)) {
         region_index = (int)i;
         break;
       }
     }
   }
  if (region_index == -1) {
    if (segment->regions.length != 1) {
      // No region for this section.  Ignore.
      return;
    }
    region_index = 0;
  }
  SegmentMemoryRegion* region = segment->regions.value.p[region_index];
  group->region = region;
}

static uint64_t RegionAllocateAddress(Linker* linker, SectionGroup* group,
                                uint64_t size, uint64_t last_segment_end) {
  SegmentMemoryRegion* region = group->region;
  assert(region != NULL);
  if (region->next == 0) {
    region->next = last_segment_end;
  }
  // The ELF writer aligns each output section's file offset.  Apply the same
  // alignment to its virtual address so the offset-to-address delta remains
  // constant throughout the containing PT_LOAD segment.
  if (region->next == group->address && group->alignment > 1) {
    region->next =
        ((region->next + group->alignment - 1) / group->alignment) *
        group->alignment;
    group->address = region->next;
  }
  // Make sure we have enough space in the region, including alignment.
  if (region->config_end != 0) {
    uint64_t next = region->next + size;
    if (next > region->config_end) {
      LinkerError(NULL, "Cannot add contents of section '%s' with size %zd to memory region '%s'",
                  group->name.value, (size_t)size, region->name.value);
      return 0;
    }
  }
  uint64_t addr = region->next;
  region->next += size;
  return addr;
}

uint64_t SegmentEndAddress(Segment* segment) {
  uint64_t end = 0;
  for (size_t i = 0; i < segment->regions.length; i++) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    uint64_t region_end =
        region->falign ? region->next : region->config_end;
    if (region->next > region_end) {
      region_end = region->next;
    }
    if (region->actual_end > region_end) {
      region_end = region->actual_end;
    }
    if (region->start > region_end) {
      region_end = region->start;
    }
    if (region_end > end) {
      end = region_end;
    }
  }
  return end;
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
  // Byte-addressed ELF load regions use LENGTH as a capacity limit, not as a
  // request to materialize the unused address space in the output file.
  // Non-file-aligned regions are fixed images (notably 6502 ROMs) and retain
  // the historical fill-to-end behavior.
  if (region->config_end != 0 && !region->falign) {
    return region->config_end - region->next;
  }
  return 0;
}

// Data for passing to map traverse function to build the section groups.
struct SectionGroupingData {
  Linker* linker;
  int32_t section_type;
};

typedef enum {
  kLinkerArrayKindNone,
  kLinkerArrayKindPreinit,
  kLinkerArrayKindInit,
  kLinkerArrayKindFini,
} LinkerArrayKind;

typedef struct LinkerArraySectionEntry {
  ELFReaderSection* section;
  size_t file_index;
  size_t section_index;
  int32_t priority;
} LinkerArraySectionEntry;

static bool LinkerParseArrayPrioritySuffix(const char* name, const char* base,
                                           int32_t default_priority,
                                           int32_t* priority) {
  size_t base_len = strlen(base);
  if (strcmp(name, base) == 0) {
    *priority = default_priority;
    return true;
  }
  if (strncmp(name, base, base_len) != 0 || name[base_len] != '.') {
    return false;
  }
  const char* suffix = name + base_len + 1;
  if (*suffix == '\0') {
    return false;
  }
  char* end = NULL;
  unsigned long value = strtoul(suffix, &end, 10);
  if (end == suffix || *end != '\0' || value > 65535) {
    return false;
  }
  *priority = (int32_t)value;
  return true;
}

static bool LinkerClassifyArraySection(ELFReaderSection* section,
                                       LinkerArrayKind* kind,
                                       int32_t* priority) {
  int32_t type = section->header->type;
  const char* name = section->name.value;

  if (type == SHT(preinit_array)) {
    *kind = kLinkerArrayKindPreinit;
    if (!LinkerParseArrayPrioritySuffix(name, ".preinit_array", 0, priority)) {
      *priority = 0;
    }
    return true;
  }
  if (type == SHT(init_array)) {
    *kind = kLinkerArrayKindInit;
    if (!LinkerParseArrayPrioritySuffix(name, ".init_array",
                                        LINKER_ARRAY_DEFAULT_PRIORITY,
                                        priority)) {
      *priority = LINKER_ARRAY_DEFAULT_PRIORITY;
    }
    return true;
  }
  if (type == SHT(fini_array)) {
    *kind = kLinkerArrayKindFini;
    if (!LinkerParseArrayPrioritySuffix(name, ".fini_array",
                                        LINKER_ARRAY_DEFAULT_PRIORITY,
                                        priority)) {
      *priority = LINKER_ARRAY_DEFAULT_PRIORITY;
    }
    return true;
  }
  if (type == SHT(progbits)) {
    if (LinkerParseArrayPrioritySuffix(name, ".ctors",
                                       LINKER_ARRAY_DEFAULT_PRIORITY,
                                       priority)) {
      *kind = kLinkerArrayKindInit;
      return true;
    }
    if (LinkerParseArrayPrioritySuffix(name, ".dtors",
                                       LINKER_ARRAY_DEFAULT_PRIORITY,
                                       priority)) {
      *kind = kLinkerArrayKindFini;
      return true;
    }
  }
  *kind = kLinkerArrayKindNone;
  return false;
}

static bool LinkerIsArrayInputSection(ELFReaderSection* section) {
  LinkerArrayKind kind;
  int32_t priority;
  return LinkerClassifyArraySection(section, &kind, &priority);
}

static int32_t LinkerArrayOutputType(LinkerArrayKind kind) {
  switch (kind) {
    case kLinkerArrayKindPreinit:
      return SHT(preinit_array);
    case kLinkerArrayKindInit:
      return SHT(init_array);
    case kLinkerArrayKindFini:
      return SHT(fini_array);
    default:
      abort();
  }
}

static const char* LinkerArrayCanonicalName(LinkerArrayKind kind) {
  switch (kind) {
    case kLinkerArrayKindPreinit:
      return ".preinit_array";
    case kLinkerArrayKindInit:
      return ".init_array";
    case kLinkerArrayKindFini:
      return ".fini_array";
    default:
      abort();
  }
}

static int64_t LinkerArrayDefaultFlags(void) {
  return SHF(alloc) | SHF(write);
}

static int64_t LinkerPointerSize(const Linker* linker) {
  if (linker->arch != NULL &&
      linker->arch->machine_type == ELF_MACHINE_TYPEW65C02) {
    return 2;
  }
  return linker->ops->is_64_bit ? 8 : 4;
}

static bool LinkerIsArrayOutputType(int32_t type) {
  return type == SHT(preinit_array) || type == SHT(init_array) ||
         type == SHT(fini_array);
}

static int CompareLinkerArraySectionEntries(const void* a, const void* b) {
  const LinkerArraySectionEntry* e1 = *(const LinkerArraySectionEntry* const*)a;
  const LinkerArraySectionEntry* e2 = *(const LinkerArraySectionEntry* const*)b;
  if (e1->priority != e2->priority) {
    return e1->priority - e2->priority;
  }
  if (e1->file_index != e2->file_index) {
    return (int)(e1->file_index - e2->file_index);
  }
  return (int)(e1->section_index - e2->section_index);
}

static void LinkerBuildArraySectionGroup(Linker* linker, LinkerArrayKind kind,
                                         Vector* entries) {
  if (entries->length == 0) {
    return;
  }

  VectorSortPointers(entries, CompareLinkerArraySectionEntries);

  ELFReaderSection* first = ((LinkerArraySectionEntry*)entries->value.p[0])->section;
  int64_t flags = first->header->flags != 0 ? first->header->flags
                                              : LinkerArrayDefaultFlags();
  int64_t alignment = first->header->addralign;
  int64_t pointer_size = LinkerPointerSize(linker);

  String canonical_name;
  StringInit(&canonical_name, LinkerArrayCanonicalName(kind));
  SectionGroup* group =
      NewSectionGroup(&canonical_name, LinkerArrayOutputType(kind), flags,
                      alignment > pointer_size ? alignment : pointer_size);
  StringDestruct(&canonical_name);

  for (size_t i = 0; i < entries->length; i++) {
    LinkerArraySectionEntry* entry = entries->value.p[i];
    ELFReaderSection* section = entry->section;
    if (section->header->addralign > group->alignment) {
      group->alignment = section->header->addralign;
    }
    if (section->header->flags != 0) {
      flags = section->header->flags;
      group->flags = flags;
    }
    GroupedSection* gsection = NewExistingGroupedSection(section);
    VectorAppend(&group->components, gsection);
  }

  VectorAppend(&linker->section_groups, group);
}

static void LinkerGroupArraySections(Linker* linker) {
  Vector preinit_entries;
  Vector init_entries;
  Vector fini_entries;
  VectorInit(&preinit_entries);
  VectorInit(&init_entries);
  VectorInit(&fini_entries);

  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    ObjectFile* file = linker->files.value.p[file_index];
    for (size_t section_index = 0; section_index < file->elf_file->sections.length;
         section_index++) {
      ELFReaderSection* section =
          file->elf_file->sections.value.p[section_index];
      LinkerArrayKind kind;
      int32_t priority;
      if (section->discarded ||
          !LinkerClassifyArraySection(section, &kind, &priority)) {
        continue;
      }

      LinkerArraySectionEntry* entry = malloc(sizeof(LinkerArraySectionEntry));
      entry->section = section;
      entry->file_index = file_index;
      entry->section_index = section_index;
      entry->priority = priority;

      switch (kind) {
        case kLinkerArrayKindPreinit:
          VectorAppend(&preinit_entries, entry);
          break;
        case kLinkerArrayKindInit:
          VectorAppend(&init_entries, entry);
          break;
        case kLinkerArrayKindFini:
          VectorAppend(&fini_entries, entry);
          break;
        default:
          free(entry);
          break;
      }
    }
  }

  LinkerBuildArraySectionGroup(linker, kLinkerArrayKindPreinit, &preinit_entries);
  LinkerBuildArraySectionGroup(linker, kLinkerArrayKindInit, &init_entries);
  LinkerBuildArraySectionGroup(linker, kLinkerArrayKindFini, &fini_entries);

  VectorDestructWithContents(&preinit_entries, NULL, /*free_element=*/true);
  VectorDestructWithContents(&init_entries, NULL, /*free_element=*/true);
  VectorDestructWithContents(&fini_entries, NULL, /*free_element=*/true);
}

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
    const char* symbol_addr = elf_file->base + symtab->header->offset;
    const ELFFormatOps* ops = elf_file->ops;
    
    // Now read the symbols and add them to the symbol tables in the file.
    for (size_t i = 0; i < num_symbols; i++) {
      // LinkerReadSymbol may retain the ELFSymbol pointer (via NewLinkerSymbol),
      // so for ELF32 we decode into a heap-allocated wide symbol.  For ELF64 the
      // on-disk layout matches the canonical struct and we alias it directly.
      ELFSymbol* elf_sym;
      if (ops->is_64_bit) {
        elf_sym = (ELFSymbol*)symbol_addr;
      } else {
        elf_sym = malloc(sizeof(ELFSymbol));
        ops->ReadSymbol(elf_sym, symbol_addr);
      }
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
    const char* symbol_table_address = elf_file->base +
    symtab->header->offset;
    const ELFFormatOps* ops = elf_file->ops;
    
    int64_t num_relocations = reloc_section->header->size /
    reloc_section->header->entsize;
    
    const char* reloc_addr = elf_file->base +
    reloc_section->header->offset;
    ELFRelocation reloc_storage;
    for (int64_t ri = 0; ri < num_relocations; ri++) {
      // Decode the on-disk relocation into a canonical (wide) relocation.  The
      // relocation is consumed immediately, so a stack temporary suffices.
      ELFFormatReadRelocation(ops,
                              reloc_section->header->type == SHT(rela),
                              &reloc_storage, reloc_addr);
      LinkerReadRelocation(linker, file, elf_file, &reloc_storage,
                           symbol_table_address, reloc_section, symtab, strtab);
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
    int64_t grouping_type = section->header->type;
    MapKeyValue kv;
    kv.key.p = &section->name;
    kv.value.p = section;
    MapInsert(&file->sections_by_name, kv);
    
    // Clang/GCC use the processor-specific SHT_X86_64_UNWIND type for
    // .eh_frame. Treat it as PROGBITS for output grouping so it is merged with
    // DaveCC and archive-provided frame records.
    if (linker->elf_machine_type == ELF_MACHINE_TYPE_X86_64 &&
        grouping_type == 0x70000001 &&
        strcmp(section->name.value, ".eh_frame") == 0) {
      grouping_type = SHT(progbits);
    }
    Vector* type_list =
        MapFindInt64Key(&file->sections_by_type, grouping_type);
    if (type_list == NULL) {
      type_list = NewVector();
      MapKeyValue kv;
      kv.key.w = grouping_type;
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
    // First file, record machine type and the ELF format (32 vs 64 bit) to
    // use for both decoding inputs and writing the output.
    linker->elf_machine_type = file->elf_file->header->machine;
    linker->elf_flags = file->elf_file->header->flags;
    linker->ops = file->elf_file->ops;
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

void LinkerAddWholeStaticLibrary(Linker* linker, const char* name) {
  size_t old_length = linker->static_libraries.length;
  LinkerAddStaticLibrary(linker, name);
  if (linker->static_libraries.length == old_length) {
    LinkerError(NULL, "Unable to read whole archive %s", name);
    return;
  }
  ARArchive* archive =
      linker->static_libraries.value.p[linker->static_libraries.length - 1];
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = archive->files.value.p[i];
    if (strcmp(file->filename.value, "/") == 0 ||
        strcmp(file->filename.value, "//") == 0) {
      continue;
    }
    if (!LinkerReadObjectFileFromArchive(linker, archive, file)) {
      LinkerError(NULL, "Unable to read %s from whole archive %s",
                  file->filename.value, name);
    }
  }
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
      if (LinkerSymbolIsWeak(symbol)) {
        continue;
      }
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
        symbol->from_dynamic_library = true;
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

  // Take the flags from the first section and preserve the strictest input
  // alignment on the combined output section.
  ELFReaderSection* first_section = sections->value.p[0];
  int64_t section_flags = first_section->header->flags;
  int64_t section_alignment = 1;
  for (size_t i = 0; i < sections->length; i++) {
    ELFReaderSection* section = sections->value.p[i];
    if (section->header->addralign > section_alignment) {
      section_alignment = section->header->addralign;
    }
  }

  SectionGroup* group = NewSectionGroup(name, section_type, section_flags,
                                        section_alignment);
  VectorAppend(&linker->section_groups, group);

  // Each input section must start at its own sh_addralign boundary inside the
  // combined output section. This is especially important for ARM atomics:
  // an unaligned 32-bit object in .bss makes ldrex/strex fault with SIGBUS.
  uint64_t group_offset = 0;
  for (size_t i = 0; i < sections->length; i++) {
    ELFReaderSection* s = sections->value.p[i];
    uint64_t alignment =
        s->header->addralign > 1 ? (uint64_t)s->header->addralign : 1;
    uint64_t aligned_offset =
        ((group_offset + alignment - 1) / alignment) * alignment;
    if (aligned_offset != group_offset) {
      ELFWriterSectionContents* padding =
          NewELFWriterSectionContents(kSectionContentsPad);
      padding->size = aligned_offset - group_offset;
      VectorAppend(&group->components, NewGroupedSectionPadding(padding));
      group_offset = aligned_offset;
    }
    GroupedSection* gsection = NewExistingGroupedSection(s);
    VectorAppend(&group->components, gsection);
    group_offset += s->header->size;
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

// Map per-function and per-object subsections onto the output section the
// linker config already knows about (.text.foo -> .text).
static const char* CanonicalSectionGroupName(const String* name) {
  if (name->length > 6 && strncmp(name->value, ".text.", 6) == 0) {
    return ".text";
  }
  if (name->length > 6 && strncmp(name->value, ".data.", 6) == 0) {
    return ".data";
  }
  if (name->length > 8 && strncmp(name->value, ".rodata.", 8) == 0) {
    return ".rodata";
  }
  if (name->length > 5 && strncmp(name->value, ".bss.", 5) == 0) {
    return ".bss";
  }
  return name->value;
}

// Group all sections with the given type into a the section_groups
// vector in the Linker.
static bool SegmentContainsSection(Segment* segment, String* section_name);

static void GroupSections(Linker* linker, int32_t section_type,
                          int32_t section_flags) {
  Map section_map;
  MapInitForStringKeys(&section_map);
  Vector owned_keys = {0};

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
      if (section->discarded) {
        continue;
      }
      if (section_flags != 0 && (section->header->flags & section_flags) == 0) {
        // An explicit linker-script placement is authoritative even for
        // metadata sections that are intentionally non-allocatable in input
        // objects (for example Xtensa's .xtensa.info).
        if (section_type != SHT(progbits) ||
            (!SegmentContainsSection(&linker->code_segment, &section->name) &&
             !SegmentContainsSection(&linker->data_segment, &section->name))) {
          continue;
        }
      }
      if (section_type == SHT(progbits) &&
          LinkerIsArrayInputSection(section)) {
        continue;
      }
      const char* group_name = CanonicalSectionGroupName(&section->name);
      String lookup;
      StringInit(&lookup, group_name);
      Vector* result_vec = MapFindPointerKey(&section_map, &lookup);
      if (result_vec == NULL) {
        String* key = NewString(group_name);
        VectorAppend(&owned_keys, key);
        result_vec = NewVector();
        MapKeyValue kv;
        kv.key.p = key;
        kv.value.p = result_vec;
        MapInsert(&section_map, kv);
      }
      StringDestruct(&lookup);
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
  for (size_t i = 0; i < owned_keys.length; i++) {
    StringDelete(owned_keys.value.p[i]);
  }
  VectorDestruct(&owned_keys);
}

static bool SegmentContainsSection(Segment* segment, String* section_name) {
  for (size_t i = 0; i < segment->config->regions.length; i++) {
    ConfigRegion* region = segment->config->regions.value.p[i];
    for (size_t j = 0; j < region->sections.length; j++) {
      String* pattern = region->sections.value.p[j];
      if (LinkerConfigPatternMatch(pattern->value, section_name->value)) {
        return true;
      }
    }
  }
  return false;
}

static SegmentMemoryRegion* SegmentRegionForSection(Segment* segment,
                                                    const char* section_name) {
  for (size_t i = 0; i < segment->regions.length; i++) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    for (size_t j = 0; j < region->sections.length; j++) {
      if (LinkerConfigPatternMatch(
              ((String*)region->sections.value.p[j])->value, section_name)) {
        return region;
      }
    }
  }
  return NULL;
}

// Now we have the sections grouped we can assign each section to its
// appropriate segment.  TLS are always in the tls_segment but other sections
// need to be assigned to a segment in the config file.
static void AssignSectionGroupsToSegments(Linker* linker) {
  SegmentMemoryRegion* code_default_region = NULL;
  SegmentMemoryRegion* data_default_region = NULL;
  for (size_t i = 0; i < linker->section_groups.length; i++) {
    SectionGroup* group = linker->section_groups.value.p[i];
    Segment* segment = NULL;
    bool use_default_region = false;
    if (LinkerConfigShouldDiscard(&linker->config, group->name.value)) {
      continue;
    }
    if ((group->flags & SHF(tls)) != 0) {
      segment = &linker->tls_segment;
    } else if (SegmentContainsSection(&linker->code_segment, &group->name)) {
      segment = &linker->code_segment;
    } else if (SegmentContainsSection(&linker->data_segment, &group->name)) {
      segment = &linker->data_segment;
    } else if ((group->flags & SHF(write)) != 0) {
      // Foreign compilers commonly use mergeable subsections such as
      // .data.DW.ref.* and .data.rel.ro.*.  Place otherwise-unconfigured
      // writable sections in the data segment.
      segment = &linker->data_segment;
      use_default_region = true;
    } else if ((group->flags & SHF(alloc)) != 0) {
      // Likewise, read-only allocatable subsections belong with code/rodata.
      segment = &linker->code_segment;
      use_default_region = true;
    }
    if (segment == NULL) {
      LinkerError(NULL, "Cannot find segment for section %s", group->name.value);
      return;
    }
    group->segment = segment;
    if (use_default_region) {
      SegmentMemoryRegion** default_region =
          segment == &linker->code_segment ? &code_default_region
                                           : &data_default_region;
      if (*default_region == NULL) {
        *default_region = SegmentRegionForSection(
            segment, segment == &linker->code_segment ? ".rodata" : ".data");
        if (*default_region == NULL) {
          *default_region = SegmentDefaultRegion(segment);
        }
      }
      group->region = *default_region;
    }
    VectorAppend(&segment->sections, group);
  }
}

static LinkerSymbol* InventSymbol(Linker* linker, const char* name, int size, uint64_t address) {
  LinkerSymbol* sym = LinkerFindSymbol(&linker->global_symbol_table, name);
  if (sym == NULL) {
    sym = LinkerInventSymbol(linker, name, size);
  }
  sym->defined = true;
  // A DSO symbol must have a regular section index so ld.so treats its value
  // as relative to the object's load bias. The precise section is immaterial
  // for these linker-computed absolute-within-image boundary values.
  sym->header->shndx = linker->building_dso ? 1 : SHN_ABS;
  sym->size = size;
  sym->address = address;
  return sym;
}

static uint64_t AlignUpU64(uint64_t value, uint64_t align) {
  if (align <= 1) {
    return value;
  }
  return ((value + align - 1) / align) * align;
}

static void ApplyScriptSymbols(Linker* linker, uint64_t image_end) {
  for (size_t i = 0; i < linker->config.script_symbols.length; i++) {
    ConfigScriptSymbol* spec = linker->config.script_symbols.value.p[i];
    LinkerSymbol* existing =
        LinkerFindSymbol(&linker->global_symbol_table, spec->name.value);
    if (spec->provide && existing != NULL && existing->defined) {
      continue;
    }
    uint64_t address = image_end;
    if (spec->has_absolute) {
      address = spec->absolute;
    } else if (!spec->image_end && spec->patterns.length > 0) {
      bool found = false;
      uint64_t end = 0;
      for (size_t g = 0; g < linker->section_groups.length; g++) {
        SectionGroup* group = linker->section_groups.value.p[g];
        if (LinkerConfigSectionMatches(&spec->patterns, group->name.value)) {
          uint64_t group_end = group->address + LinkerSectionGroupSize(group);
          if (!found || group_end > end) {
            end = group_end;
            found = true;
          }
        }
      }
      if (found) {
        address = end;
      }
    }
    address = AlignUpU64(address, spec->align);
    InventSymbol(linker, spec->name.value, 8, address);
  }
}

static SectionGroup* FindSectionGroup(Linker* linker, const char* name) {
  return LinkerFindSectionGroup(linker, name);
}

SectionGroup* LinkerFindSectionGroup(Linker* linker, const char* name) {
  for (size_t i = 0; i < linker->section_groups.length; i++) {
    SectionGroup* group = linker->section_groups.value.p[i];
    if (strcmp(group->name.value, name) == 0) {
      return group;
    }
  }
  return NULL;
}

static uint64_t SectionGroupSize(SectionGroup* group) {
  return LinkerSectionGroupSize(group);
}

uint64_t LinkerSectionGroupSize(SectionGroup* group) {
  uint64_t size = 0;
  for (size_t i = 0; i < group->components.length; i++) {
    GroupedSection* section = group->components.value.p[i];
    switch (section->source) {
      case kGroupedSectionExisting:
        size += section->section.existing->header->size;
        break;
      case kGroupedSectionNew:
        size += section->section.new->contents->size;
        break;
      case kGroupedSectionPadding:
        size += section->section.padding->size;
        break;
    }
  }
  return size;
}

// The span of address space the TLS initialization image occupies inside the
// writable segment, measured from the end of the initialized data.
//
// Only the part of the TLS block that has file content counts.  The writer
// aligns each output section's file offset by that section's own alignment and
// gives a load segment's address and file offset the same residue modulo the
// segment alignment, so walking the TLS sections in address space reproduces
// the gaps the file layout will have.  A nobits section takes the current
// offset without advancing it, so it contributes nothing.  A block built from
// .tbss alone has no image at all, and gets no room: the writer leaves the load
// segment's file size alone in that case, which is what keeps it from copying
// the padding after the initialized sections over the start of .bss.
static uint64_t LinkerTLSImageSpan(Linker* linker, uint64_t data_end) {
  uint64_t file_backed = 0;
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    SectionGroup* group = linker->tls_segment.sections.value.p[i];
    if (group->type != SHT(nobits)) {
      file_backed += LinkerSectionGroupSize(group);
    }
  }
  if (file_backed == 0) {
    return 0;
  }
  uint64_t addr = data_end;
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    SectionGroup* group = linker->tls_segment.sections.value.p[i];
    if (group->type == SHT(nobits)) {
      continue;
    }
    uint64_t alignment = group->alignment > 1 ? (uint64_t)group->alignment : 1;
    addr = (addr + alignment - 1) & ~(alignment - 1);
    addr += LinkerSectionGroupSize(group);
  }
  return addr - data_end;
}

static void InventEHFrameBounds(Linker* linker) {
  SectionGroup* eh_frame = FindSectionGroup(linker, ".eh_frame");
  uint64_t start = 0;
  uint64_t end = 0;
  if (eh_frame != NULL && eh_frame->region != NULL) {
    start = eh_frame->address;
    end = start + SectionGroupSize(eh_frame);
  }
  InventSymbol(linker, "__eh_frame_start", 8, start);
  InventSymbol(linker, "__eh_frame_end", 8, end);
}

static void InventGccExceptTableBounds(Linker* linker) {
  SectionGroup* table = FindSectionGroup(linker, ".gcc_except_table");
  uint64_t start = 0;
  uint64_t end = 0;
  if (table != NULL && table->region != NULL) {
    start = table->address;
    end = start + SectionGroupSize(table);
  }
  InventSymbol(linker, "__gcc_except_table_start", 8, start);
  InventSymbol(linker, "__gcc_except_table_end", 8, end);
}

static void InventExceptionTableBounds(Linker* linker) {
  if (linker->elf_machine_type != ELF_MACHINE_TYPE_PCODE) {
    return;
  }
  SectionGroup* table = FindSectionGroup(linker, ".davecc_except_table");
  uint64_t start = 0;
  uint64_t end = 0;
  if (table != NULL && table->region != NULL) {
    start = table->address;
    end = start + SectionGroupSize(table);
  }
  InventSymbol(linker, "__davecc_except_table_start", 8, start);
  InventSymbol(linker, "__davecc_except_table_end", 8, end);
}

static void InventARMExidxBounds(Linker* linker) {
  if (linker->elf_machine_type != ELF_MACHINE_TYPE_ARM) {
    return;
  }
  SectionGroup* exidx = FindSectionGroup(linker, ".ARM.exidx");
  uint64_t start = 0;
  uint64_t end = 0;
  if (exidx != NULL && exidx->region != NULL) {
    start = exidx->address;
    end = start + SectionGroupSize(exidx);
  }
  InventSymbol(linker, "__exidx_start", 4, start);
  InventSymbol(linker, "__exidx_end", 4, end);

  SectionGroup* extab = FindSectionGroup(linker, ".ARM.extab");
  start = 0;
  end = 0;
  if (extab != NULL && extab->region != NULL) {
    start = extab->address;
    end = start + SectionGroupSize(extab);
  }
  InventSymbol(linker, "__extab_start", 4, start);
  InventSymbol(linker, "__extab_end", 4, end);
}

typedef struct {
  uint64_t function;
  uint64_t unwind;
  bool cant_unwind;
  bool compact_inline;
} ARMExidxRecord;

static COMPILER_UNUSED int CompareARMExidxRecord(const void* a, const void* b) {
  const ARMExidxRecord* left = a;
  const ARMExidxRecord* right = b;
  return left->function < right->function
             ? -1
             : (left->function > right->function ? 1 : 0);
}

static COMPILER_UNUSED uint64_t DecodeARMPrel31(uint64_t place,
                                                uint32_t value) {
  int32_t offset = (int32_t)(value << 1) >> 1;
  return place + (int64_t)offset;
}

static COMPILER_UNUSED uint32_t EncodeARMPrel31(uint64_t place,
                                                uint64_t target) {
  return (uint32_t)((int64_t)target - (int64_t)place) & 0x7fffffffu;
}

static void SortARMExidx(Linker* linker) {
  (void)linker;
  /* Relocations already produce correct PREL31 exidx entries.  Re-encoding
   * after qsort was scrambling compact-inline second words and mismapping
   * sorted records back onto component order.  Lookup scans all entries. */
}

static void LinkerInventArrayBoundsSymbols(Linker* linker) {
  static const struct {
    const char* section_name;
    const char* start_symbol;
    const char* end_symbol;
  } kArrayBounds[] = {
      {".preinit_array", "__preinit_array_start", "__preinit_array_end"},
      {".init_array", "__init_array_start", "__init_array_end"},
      {".fini_array", "__fini_array_start", "__fini_array_end"},
  };

  for (size_t i = 0; i < sizeof(kArrayBounds) / sizeof(kArrayBounds[0]); i++) {
    SectionGroup* group = LinkerFindSectionGroup(linker, kArrayBounds[i].section_name);
    uint64_t start = 0;
    uint64_t end = 0;
    if (group != NULL && group->region != NULL) {
      start = group->address;
      end = start + LinkerSectionGroupSize(group);
    }
    InventSymbol(linker, kArrayBounds[i].start_symbol, 8, start);
    InventSymbol(linker, kArrayBounds[i].end_symbol, 8, end);
  }
  if (linker->building_dso) {
    uint64_t empty_array_address = SegmentEndAddress(&linker->code_segment);
    SectionGroup* init = LinkerFindSectionGroup(linker, ".init_array");
    uint64_t init_start =
        init != NULL ? init->address : empty_array_address;
    uint64_t init_end =
        init != NULL ? init_start + LinkerSectionGroupSize(init)
                     : empty_array_address;
    InventSymbol(linker, "__davecc_dso_init_array_start", 8, init_start);
    InventSymbol(linker, "__davecc_dso_init_array_end", 8, init_end);

    SectionGroup* fini = LinkerFindSectionGroup(linker, ".fini_array");
    uint64_t fini_start =
        fini != NULL ? fini->address : empty_array_address;
    uint64_t fini_end =
        fini != NULL ? fini_start + LinkerSectionGroupSize(fini)
                     : empty_array_address;
    InventSymbol(linker, "__davecc_dso_fini_array_start", 8, fini_start);
    InventSymbol(linker, "__davecc_dso_fini_array_end", 8, fini_end);
  }
}

static int SectionOrderInRegion(const SectionGroup* group) {
  if (group->region == NULL) {
    return INT_MAX;
  }
  for (size_t i = 0; i < group->region->sections.length; i++) {
    String* configured_name = group->region->sections.value.p[i];
    if (LinkerConfigPatternMatch(configured_name->value, group->name.value)) {
      return (int)i;
    }
  }
  return INT_MAX;
}

static int CompareGroupRegion(const void* a, const void* b) {
  const SectionGroup* g1 = *(const SectionGroup**)a;
  const SectionGroup* g2 = *(const SectionGroup**)b;
  uint64_t start1 = g1->region != NULL ? g1->region->start : 0;
  uint64_t start2 = g2->region != NULL ? g2->region->start : 0;
  if (start1 != start2) {
    if (start1 < start2) {
      return -1;
    }
    return 1;
  }
  return SectionOrderInRegion(g1) - SectionOrderInRegion(g2);
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
  SectionGroup* first_output_group = NULL;
  int64_t segment_section_alignment = 1;
  for (size_t i = 0; i < segment->sections.length; i++) {
    SectionGroup* group = segment->sections.value.p[i];
    if (group->region == NULL && (group->flags & SHF(tls)) == 0) {
      continue;
    }
    if (first_output_group == NULL) {
      first_output_group = group;
    }
    if (group->alignment > segment_section_alignment) {
      segment_section_alignment = group->alignment;
    }
  }
  if (first_output_group != NULL &&
      first_output_group->alignment < segment_section_alignment) {
    // Give the segment's file offset and virtual address the same residue for
    // every alignment required by a section later in the segment.
    first_output_group->alignment = segment_section_alignment;
  }
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
      if ((group->flags & SHF(tls)) == 0) {
        // No region for this group, it's not in the output.
        continue;
      }
      // TLS template sections are not mapped into a PT_LOAD region, but they
      // still need consecutive offsets for local-exec relocations and PT_TLS.
      group->address = (uint64_t)offset;
      uint64_t tls_addr = (uint64_t)offset;
      for (size_t j = 0; j < group->components.length; j++) {
        GroupedSection* gsect = group->components.value.p[j];
        switch (gsect->source) {
          case kGroupedSectionExisting: {
            ELFReaderSection* section = gsect->section.existing;
            section->address = tls_addr;
            section->offset = offset;
            offset += section->header->size;
            tls_addr += section->header->size;
            break;
          }
          case kGroupedSectionNew: {
            ELFWriterSection* section = gsect->section.new;
            section->address = tls_addr;
            tls_addr += section->contents->size;
            break;
          }
          case kGroupedSectionPadding: {
            uint64_t size = gsect->section.padding->size;
            offset += size;
            tls_addr += size;
            break;
          }
        }
      }
      continue;
    }
    if (addr == 0) {
      uint64_t start_addr = AlignedStartAddress(group->region, file_offset);
      SetSegmentStartAddress(segment, start_addr);
      addr = start_addr;
    }
    group->address = RegionNextAddress(group);
    if (group->address == 0) {
      group->address = addr != 0 ? addr : last_segment_end;
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
          addr = section->address + section->header->size;
          break;
        }
        case kGroupedSectionNew: {
          ELFWriterSection* section = gsect->section.new;
          section->address = RegionAllocateAddress(linker,
                                                    group,
                                                    section->contents->data.buffered.length,
                                                    addr);
          addr = section->address + section->contents->size;
          break;
        }
        case kGroupedSectionPadding: {
          uint64_t size = gsect->section.padding->size;
          uint64_t padding_address =
              RegionAllocateAddress(linker, group, size, addr);
          offset += size;
          addr = padding_address + size;
          break;
        }
        
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
  
  for (size_t i = 0; i < segment->regions.length; i++) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    if (region->trailing_align > 1) {
      uint64_t aligned =
          ((region->next + region->trailing_align - 1) /
           region->trailing_align) *
          region->trailing_align;
      region->next = aligned;
      if (aligned > region->actual_end) {
        region->actual_end = aligned;
      }
      if (aligned > addr) {
        addr = aligned;
      }
    }
  }
  SetSegmentEndAddress(segment, addr);
}

static ConfigSegment* FakeConfigSegment() {
  ConfigSegment* s = malloc(sizeof(ConfigSegment));
  VectorInit(&s->regions);
  return s;
}

static void ClearSegmentFixedAddresses(Segment* segment) {
  for (size_t i = 0; i < segment->regions.length; i++) {
    SegmentMemoryRegion* region = segment->regions.value.p[i];
    region->start = 0;
    region->next = 0;
    region->actual_end = 0;
    region->config_end = 0;
  }
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
  if (linker->building_dso) {
    // Shared objects use position-independent virtual addresses. The default
    // program layouts contain fixed executable addresses, so discard those
    // bases and lay the DSO segments out consecutively from its ELF headers.
    ClearSegmentFixedAddresses(&linker->code_segment);
    ClearSegmentFixedAddresses(&linker->dynamic_segment);
    ClearSegmentFixedAddresses(&linker->data_segment);
  }
  // Resolve all undefined symbols in libraries.
  ResolveUndefinedSymbols(linker);
  // Archive members are only materialized while resolving undefined
  // symbols, so collect their GOT and PLT relocations afterward.  A static
  // link needs this too: position-independent code reads variable addresses
  // out of the GOT whether or not there is a loader to relocate them.
  DynamicLinkerGatherDynamicRelocations(linker);
  bool static_got = DynamicLinkerNeedsStaticGOT(linker);

  // Drop unreferenced allocatable input sections before they are merged
  // into output section groups.  Per-function .text.* pieces stay distinct
  // until this point so an unused function can be discarded on its own.
  LinkerGarbageCollectSections(linker);
  
  // Find all PROGBITS sections and group by name.  These are sections
  // that have data associated with them in the ELF file.  This also
  // adds the grouped sections to the appropriate segment (code, data or tls).
  GroupSections(linker, SHT(progbits), SHF(alloc));

  if (linker->elf_machine_type == ELF_MACHINE_TYPE_XTENSA) {
    // Xtensa objects carry ABI and literal-pool policy in .xtensa.info.
    // Preserve that non-allocatable NOTE section in linked output.
    GroupSections(linker, SHT(note), 0);
  }

  if (linker->elf_machine_type == ELF_MACHINE_TYPE_ARM) {
    GroupSections(linker, SHT(ARM_EXIDX), 0);
  }

  // Group init/fini/preinit array sections (including legacy .ctors/.dtors).
  LinkerGroupArraySections(linker);
  
  // Group the TLS nobits sections.
  GroupSections(linker, SHT(nobits), SHF(tls));

  // Add compact runtime symbol metadata only when stacktrace support is linked.
  LinkerStacktracePrepare(linker);

  // Assign all section groups to their appropriate segments.
  AssignSectionGroupsToSegments(linker);
  
  // Create all the loadable sections for a dynamic library.  We need
  // to do this now so that the addresses of the sections can be calculated.
  // However, not all the information is available at this point so we
  // put placeholders in the section contents.
  if (!linker->fully_static) {
    // Make linker-defined boundary symbols visible while the dynamic symbol
    // table is collected. Their addresses are refreshed after layout.
    InventEHFrameBounds(linker);
    InventGccExceptTableBounds(linker);
    InventExceptionTableBounds(linker);
    InventARMExidxBounds(linker);
    LinkerInventArrayBoundsSymbols(linker);
    DynamicLinkerCreateDynamicLinkerGroups(linker);
  } else if (static_got) {
    DynamicLinkerCreateStaticGOTGroups(linker);
  }
  
  // Assign addresses to all sections.
 
  int num_program_headers = 2;  // Code and data PT_LOAD segments.
  if (!linker->building_dso) {
    num_program_headers++;  // PT_PHDR.
  }
  if (linker->tls_segment.sections.length != 0) {
    num_program_headers++;
  }
  if (!linker->fully_static) {
    num_program_headers++;  // PT_DYNAMIC.
    if (!linker->building_dso) {
      num_program_headers++;  // PT_INTERP.
    }
  }
  // NULL, .bss, .symtab, .strtab, and .shstrtab are not represented by
  // section groups.
  int num_sections = (int)linker->section_groups.length + 5;
  
  uint64_t text_file_offset = linker->ops->header_size +
  linker->ops->program_header_size * num_program_headers +
  linker->ops->section_header_size * num_sections;
  
  // Assign code segment addresses.
  AssignSegmentSectionAddresses(linker, &linker->code_segment, 0, text_file_offset);

  uint64_t end_of_segments =  SegmentEndAddress(&linker->code_segment);
  if (!linker->fully_static) {
    // Assign addresses to the dynamic section.
    uint64_t dynamic_start = end_of_segments;
    if (linker->building_dso) {
      uint64_t alignment = linker->data_segment.config->alignment;
      dynamic_start = (dynamic_start + alignment - 1) & ~(alignment - 1);
    }
    AssignSegmentSectionAddresses(linker, &linker->dynamic_segment,
                                  dynamic_start, 0);
    end_of_segments = SegmentEndAddress(&linker->dynamic_segment);
  }
  
  if (!linker->fully_static && !linker->building_dso) {
    // Assign addresses to the interpreter section.
    AssignSegmentSectionAddresses(linker, &linker->interpreter_segment, end_of_segments, 0);
    end_of_segments = SegmentEndAddress(&linker->interpreter_segment);
  }
  
  // Define the '_etext' symbol for the last assigned address.
  InventSymbol(linker, "_etext", 8, end_of_segments);
  uint64_t addr = end_of_segments;

  // Assign addresses to sections in the data segment.  This must be
  // last since it also needs to contain the .bss section.
  uint64_t data_file_offset = text_file_offset ;

  // The dynamic and data section groups share one writable PT_LOAD. Keep
  // their addresses contiguous with their file contents; page-aligning the
  // data groups here would create an unmapped virtual/file offset gap inside
  // that segment.
  uint64_t data_alignment = 8;
  AssignSegmentSectionAddresses(
      linker, &linker->data_segment,
      (end_of_segments + data_alignment - 1) & ~(data_alignment - 1),
      data_file_offset);

  // The TLS segment starts at address 0 and doesn't increment the current
  // address.
  AssignSegmentSectionAddresses(linker, &linker->tls_segment, SegmentEndAddress(&linker->data_segment), 0);
 
  // Define the '_edata' symbol for the last assigned address.
  InventSymbol(linker, "_edata", 8, SegmentEndAddress(&linker->data_segment));

  // A TLS symbol's value is its offset within the TLS block, which is what a
  // local-exec relocation needs, so the TLS sections cannot also carry a load
  // address and are skipped when addresses are handed out.  Their file content
  // is still written immediately after the initialized data, and an executable
  // stretches the writable PT_LOAD to cover it so a native loader can reach the
  // image, so the loader copies the image to whatever address that file offset
  // maps to.  Nothing claimed that address, and .bss started there, so the
  // image landed on the first bytes of .bss.  Reserve the image its own range
  // here, before .bss is placed.  Only an executable remaps PT_TLS this way; a
  // DSO leaves its PT_TLS address alone and never stretches the load segment
  // over the image, so it needs no room.
  if (!linker->building_dso) {
    uint64_t data_end = SegmentEndAddress(&linker->data_segment);
    uint64_t tls_image_span = LinkerTLSImageSpan(linker, data_end);
    if (tls_image_span != 0) {
      SetSegmentEndAddress(&linker->data_segment, data_end + tls_image_span);
    }
  }

  // Now that we know the addresses of the sections we can work
  // out the values of the symbols within those sections.
  LinkerAssignSymbolAddresses(linker);
  
  // Assign section symbol addresses.
  LinkerAssignSectionSymbolAddresses(linker);

  // Function addresses are now final, so materialize the stacktrace table.
  LinkerStacktraceFinalize(linker);
  
  // The .bss (nobits) address is just after initialized data.  A
  // program with no writable .data (typical 65C02 C: text+.rodata only)
  // leaves the data segment end at 0, because an empty segment never
  // gets a start address.  Commons still need a home; place them after
  // the code/rodata image so __free_list and friends are not linked at
  // zp address 0/1.
  linker->nobits_address = SegmentEndAddress(&linker->data_segment);
  if (linker->nobits_address == 0) {
    linker->nobits_address = SegmentEndAddress(&linker->code_segment);
  }
  addr = linker->nobits_address;
  LinkerAssignCommonSymbolAddresses(linker, &addr);
  linker->nobit_size = addr - linker->nobits_address;
  LinkerAssignBSSSymbolAddresses(linker);

  // Define the '_end' symbol for the last assigned address.
  InventSymbol(linker, "_end", 8, addr);
  ApplyScriptSymbols(linker, addr);

  // Expose the linked .eh_frame range to the in-process unwind runtime.
  InventEHFrameBounds(linker);
  InventGccExceptTableBounds(linker);
  InventExceptionTableBounds(linker);
  InventARMExidxBounds(linker);
  LinkerInventArrayBoundsSymbols(linker);
  
  if (!linker->fully_static) {
    // Define the dynamic linker symbols.  This includes
    // _GLOBAL_OFFSET_TABLE_ and _DYNAMIC_.
    DynamicLinkerDefineSymbols(linker);
    
    // Fixup the GOT and PLT now that we have all the addresses.
    DynamicLinkerFixupGOT(linker);
    DynamicLinkerResolveDataGOT(linker);
    DynamicLinkerFixupPLT(linker);
  } else if (static_got) {
    // No loader will relocate these slots, so give them their final values.
    DynamicLinkerDefineStaticGOTSymbol(linker);
    DynamicLinkerResolveDataGOT(linker);
  }
  
  if (linker->print_symbol_tables) {
    LinkerPrintSymbolTables(linker);
  }
  
  if (linker->elf_machine_type == ELF_MACHINE_TYPEW65C02) {
    uint64_t image_end = SegmentEndAddress(&linker->code_segment);
    uint64_t data_end = SegmentEndAddress(&linker->data_segment);
    if (data_end > image_end) {
      image_end = data_end;
    }
    if (addr > image_end) {
      image_end = addr;
    }
    if (image_end > 0x10000) {
      ObjectFile* file =
          linker->files.length == 0 ? NULL : linker->files.value.p[0];
      LinkerError(file,
                  "65C02 image ends at 0x%" PRIx64
                  " and exceeds the 64 KiB address space",
                  image_end);
      if (file == NULL) {
        linker->num_errors++;
      }
      return;
    }
  }

  // We have all the values of the symbols, apply those values to
  // all the relocations in the files.
  LinkerApplyAllRelocations(linker);
  SortARMExidx(linker);

  if (!linker->building_dso) {
    // Check if we have any undefined symbols and report errors if found.
    LinkerCheckForUndefinedSymbols(linker);
  }
}

// Build an output section from a group of sections.  Each component of the
// group is a GroupedSection that can come from an existing file or can
// be generated by the linker.
static void BuildOutputSection(Linker* linker, ELFWriterFile* elf, Segment* segment,
                                            SectionGroup* group) {
  (void)segment;
  if (group->region == NULL && (group->flags & SHF(tls)) == 0) {
    return;
  }
  ELFWriterSectionContents* contents = NewELFWriterSectionContents(kSectionContentsMulti);
  ELFWriterSection* section = ELFWriterAddSection(elf, &group->name,
                      group->type,
                      group->flags,
                      group->alignment,
                      contents, group->address);
  if (LinkerIsArrayOutputType(group->type)) {
    section->header.entsize = LinkerPointerSize(linker);
  }
  for (size_t i = 0; i < group->components.length; i++) {
    GroupedSection* gsect = group->components.value.p[i];
    ELFWriterSectionContents* part_contents;
    switch (gsect->source) {
      case kGroupedSectionExisting: {
        ELFReaderSection* part = gsect->section.existing;
        if (part->header->type != SHT(nobits)) {
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
        if (gsect->section.new->header.info != 0) {
          section->header.info = gsect->section.new->header.info;
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
  if (group->components.length == 0 ||
      (group->region == NULL && (group->flags & SHF(tls)) == 0)) {
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
    if (sym->section != NULL && sym->section->discarded) {
      continue;
    }
    if (!sym->defined) {
      section_index = 0;
    } else if (sym->section == NULL) {
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
    BuildOutputSection(linker, elf, &linker->code_segment, linker->code_segment.sections.value.p[i]);
  }
  
  if (!linker->fully_static) {
    for (size_t i = 0; i < linker->dynamic_segment.sections.length; i++) {
      BuildOutputSection(linker, elf, &linker->dynamic_segment, linker->dynamic_segment.sections.value.p[i]);
    }
  }
  
  if (!linker->fully_static && !linker->building_dso) {
    for (size_t i = 0; i < linker->interpreter_segment.sections.length; i++) {
      BuildOutputSection(linker, elf, &linker->interpreter_segment, linker->interpreter_segment.sections.value.p[i]);
    }
  }
  
  // Build output sections in data segment.
  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    BuildOutputSection(linker, elf, &linker->data_segment, linker->data_segment.sections.value.p[i]);
  }
  
  // Build output sections in tls segment.
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    BuildOutputSection(linker, elf, &linker->tls_segment, linker->tls_segment.sections.value.p[i]);
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
    
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".dynsym", ".dynstr");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".gnu_hash",
                                   ".dynsym");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, ".dynamic",
                                   ".dynstr");
    bool uses_rel = linker->elf_machine_type == ELF_MACHINE_TYPE_ARM ||
                    linker->elf_machine_type == ELF_MACHINE_TYPE_X86;
    const char* dyn_relocations = uses_rel ? ".rel.dyn" : ".rela.dyn";
    const char* plt_relocations = uses_rel ? ".rel.plt" : ".rela.plt";
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, dyn_relocations,
                                   ".dynsym");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldLink, plt_relocations,
                                   ".dynsym");
    ELFWriterAddSectionFixupByName(elf, kFixupFieldInfo, plt_relocations,
                                   ".plt");
  }
  
  for (size_t i = 0; i < linker->data_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->data_segment.sections.value.p[i], &section_index);
  }
  
  for (size_t i = 0; i < linker->tls_segment.sections.length; i++) {
    AssignGroupSectionIndexes(linker->tls_segment.sections.value.p[i], &section_index);
  }

  if (!linker->fully_static) {
    // Dynamic symbols need the final output indexes of data and TLS sections,
    // so populate .dynsym only after every allocatable group is indexed.
    ELFWriterSection* bss = ELFWriterFindSection(elf, ".bss");
    int32_t bss_section_index =
        bss != NULL ? bss->index : 0;
    DynamicLinkerFixupDynamicSymbolTable(
        linker, FindDynamicSymbolTableBuffer(elf), bss_section_index);
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
      ELFWriterSegmentAddSection(linker->building_dso ? data_segment
                                                       : code_segment,
                                 section);
    } else if (StringEqual(&section->name, ".interp")) {
      ELFWriterSegmentAddSection(interpreter_segment, section);
      ELFWriterSegmentAddSection(code_segment, section);
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
                    linker->ops->is_64_bit, true);
  
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

