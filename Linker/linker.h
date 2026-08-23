//
//  linker.h
//  c_compiler
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef linker_h
#define linker_h

#include <stdio.h>
#include "elf_reader.h"
#include "elf_writer.h"
#include "hashtable.h"
#include "map.h"
#include "ar.h"
#include "linker_config.h"

// The linker wants to create a set of program segments.  The following
// segments are created:
// 1. Code segment - consisting of all read-only and executable content.
// 2. Data segment - consisting of all read-write segments.
//
// The input data for the linker is a set of ELF files containing sections.  Each
// section has a name, type and set of flags (among other things).  We want to group
// all the sections that comprise the program executable together in the code
// segment.  These sections all have type PROGBITS and are read-only (and possibly
// executable).
//
// The sections that are writeable are grouped together into the data segment.
//
// Sections are grouped by name.  For example all sections called ".text" are grouped
// together, as are ".rodata", etc.

// The SectionGroup maps a name to a set of ELFReaderSection* components, all
// of which share the name.  For example the SectionGroup with name ".text" will
// have, as components, all the ELFReaderSections from the object files that
// are called ".text".

// The output is an ELF file.  We need to know ahead of time how many segments
// and sections we are going to be writing to output file so that we can
// assign the addresses for the sections, and therefore all the symbols
// before we perform the relocations.

// The ELF file generated is laid out as follows:

//      +--------------------------------+    <-- 0
//      |         ELF file header        |
//      +--------------------------------+    <-- x = 64
//      |      Program segment headers   |
//      +--------------------------------+    <-- y = x + num_segments * segment header size
//      |       Section headers          |
//      +--------------------------------+    <-- z = y + num_sections * section header size
//      |        Section 1               |
//      +--------------------------------+    <-- x + size of section 1 contents
//      |        Section 2               |
//      +--------------------------------+
//      |           ...                  |
//      +--------------------------------+
//      |        Section n               |
//      +--------------------------------+

// We will generate 3 program segments:
// 0: PHDR segment containing the program headers
// 1: LOAD segment for code (r+x) spanning all code sections.
// 2: LOAD segment for data (r+w) spanning all data sections

// Unfortunately we don't know how many sections there are going
// to be until we read in all the object files.  Aside from the
// PROGBITS and NOBITS sections we also have:
//
// 1. NULL
// 2. SYMTAB
// 3. STRTAB
// 4. STRTAB for section names.


#define LINKER_NUM_EXTRA_SECTIONS 4

// So after we read in the object files and know how many sections
// we can calculate the section addresses.

// On Mac OS X, the system reserves all addresses below 4GB.
// RISC-V only has 32-bit offsets from PC so we need to keep
// all symbols within 32 bits of the code.
#if defined(__APPLE__)
#define LINKER_CODE_SEGMENT_START_ADDRESS 0x400000000LL
#define LINKER_DATA_SEGMENT_START_ADDRESS 0x410000000LL
#define LINKER_SEGMENT_ALIGNMENT 0x1000LL
#elif defined(__linux__)
#define LINKER_CODE_SEGMENT_START_ADDRESS 0x40000000LL
#define LINKER_DATA_SEGMENT_START_ADDRESS 0x41000000LL
#define LINKER_SEGMENT_ALIGNMENT 0x1000LL
#else
#error "Unknown operating system"
#endif

// For a Dynamic Shared Object the addresses are not absolute.
#define LINKER_DSO_CODE_SEGMENT_START_ADDRESS 0LL
#define LINKER_DYNAMIC_SEGMENT_ALIGNMENT 0x1000LL

#define LINKER_NUM_SEGMENTS 3


struct Segment;
struct Linker;
struct LinkerSymbol;
struct ObjectFile;
struct DynamicLinker;
struct Architecture;
struct SegmentMemoryRegion;

typedef enum {
  kGroupedSectionExisting,
  kGroupedSectionNew,
  kGroupedSectionPadding,
} GroupedSectionSource;

typedef struct {
  GroupedSectionSource source;
  union {
    ELFReaderSection* existing;   // A section from an existing file.
    ELFWriterSection* new;        // A new section.
    ELFWriterSectionContents* padding;
  } section;
} GroupedSection;

typedef struct SectionGroup {
  String name;
  Vector components;          // Vector of pointers to GroupedSection.
  struct Segment* segment;
  int32_t type;
  int64_t flags;
  int64_t alignment;
  uint64_t address;
  struct SegmentMemoryRegion* region;
} SectionGroup;

SectionGroup* NewSectionGroup(const String* name, int32_t type,
                              int64_t flags, int64_t alignment);
void SectionGroupDestruct(SectionGroup* group);
void SectionGroupDelete(SectionGroup* group);

GroupedSection* NewExistingGroupedSection(ELFReaderSection* section);
GroupedSection* NewGroupedSection(ELFWriterSection* section);
void GroupedSectionDestruct(GroupedSection* g);

typedef struct SegmentMemoryRegion {
  String name;
  Vector sections;    // Names of sections in this region.
  uint64_t start;
  bool falign;                // Align to file offset.
  uint64_t config_end;       // Configured end address or 0 for unlimited.
  uint64_t actual_end;              // End address calculated by linker
  uint64_t next;      // Next address to use.
} SegmentMemoryRegion;

void SegmentMemoryRegionInit(SegmentMemoryRegion* region, ConfigRegion* config);
SegmentMemoryRegion* NewSegmentMemoryRegion(ConfigRegion* config);
void SegmentMemoryRegionDestruct(SegmentMemoryRegion* region);
void SegmentMemoryRegionDelete(SegmentMemoryRegion* region);
SegmentMemoryRegion* NewInternalSegmentMemoryRegion(void);

// A segment.  This consists of a set of sections.
typedef struct Segment {
  ConfigSegment* config;
  Vector sections;      // Vector of SectionGroup*.
  Vector regions;       // Vector of SegmentMemoryRegion*.
} Segment;

void SegmentInit(Segment* segment, ConfigSegment* config);
void SegmentDestruct(Segment* segment);

// Allocate an address from the given segment.
uint64_t SegmentAllocateAddress(struct Linker* linker, Segment* segment,
                                SectionGroup* group,
                                uint64_t size,
                                uint64_t last_segment_end);

SegmentMemoryRegion* SegmentDefaultRegion(Segment* segment);

typedef struct Linker {
  String output_filename;     // Output filename.
  Vector files;               // All object files.
  Vector architectures;       // Vector of LinkerArchitecture*.
  struct LinkerArchitecture* arch;     // Current architecture.
  Vector rpath;               // Vector of String*.
  String interpreter;         // Interpreter name.
  HashTable global_symbol_table;  // All global symbols.
  Vector section_groups;      // All SectionGroups.
  Segment code_segment;       // Executable code.
  Segment data_segment;       // Static data.
  Segment dynamic_segment;    // Dynamic library information.
  Segment interpreter_segment; // Interpreter name.
  Segment tls_segment;        // Thread local storage segment.
  uint64_t nobits_address;    // Address of .bss.
  size_t nobit_size;          // Size of .bss.
  Vector library_search_path; // Vector of String*.
  Vector static_libraries;    // Vector of ARArchive*.
  Vector dynamic_libraries;   // Vector of LoadedDynamicLibrary*.
  int elf_machine_type;       // From first object file.
  int elf_flags;              // From first object fie.
  // ELF format ops (ELF32 or ELF64) for the output and for decoding input
  // object files.  Taken from the first object file read.
  const ELFFormatOps* ops;
  bool building_dso;          // True if building a shared object.
  bool fully_static;          // Generating fully static executable.
  bool bind_now;              // Resolve all dynamic symbols before entry.
  bool defer_program_init;    // Let the program entry run executable arrays.
  struct DynamicLinker* dynamic_linker;
  Vector needed_libraries;    // Vector of String*.
  int so_name;                // Index into dynstr or -1.
  String entry_symbol;        // Symbol to use for entry point.
  int64_t origin;             // Origin address (or zero for default).
  void* stacktrace_info;      // Linker-owned stacktrace metadata state.
  
  // Debug printing.
  bool print_symbol_tables;
  bool print_relocations;
  bool print_sections;
  
  ConfigParser config_parser;
  LinkerConfig config;
  
  int num_errors;
} Linker;

// File offset of the section header table: the ELF file header followed by the
// program segment headers.  Uses the output format's on-disk sizes (ELF32 or
// ELF64).
static inline uint64_t LinkerSectionHeaderOffset(const Linker* linker) {
  return linker->ops->header_size +
         LINKER_NUM_SEGMENTS * linker->ops->program_header_size;
}

void LinkerInit(Linker* linker);
void LinkerDestruct(Linker* linker);
void LinkerInitDynamic(Linker* linker);

bool LinkerReadObjectFile(Linker* linker, String* filename);
void LinkerLinkAllFiles(Linker* linker);
bool LinkerWriteOutput(Linker* linker, FILE* fp);

struct LinkerSymbol* LinkerFindSymbol(HashTable* symbol_table,
                               const char* name);

void LinkerInsertSymbol(HashTable* symbol_table,
                        struct LinkerSymbol* sym);

void LinkerAddLibrarySearchDir(Linker* linker, String* dir);
bool LinkerAddLibrary(Linker* linker, const char* name);
void LinkerAddStaticLibrary(Linker* linker, const char* name);
void LinkerAddWholeStaticLibrary(Linker* linker, const char* name);
void LinkerAddDynamicLibrary(Linker* linker, const char* name);
bool LinkerFindSymbolInStaticLibraries(Linker* linker, const char* name,
                                 ARArchive** archive,
                                 ARFile** file);

void LinkerError(struct ObjectFile* file, const char* error, ...);
void VLinkerError(struct ObjectFile* file, const char* error, va_list ap);

void LinkerWarning(struct ObjectFile* file, const char* warn, const char* error, ...);
void VLinkerWarning(struct ObjectFile* file, const char* warn, const char* error, va_list ap);

void LinkerInitArchitecture(Linker* linker);

void LinkerInitConfigLayout(Linker* linker,
                            const char* config_file,
                            const char* layout_type_name);

SectionGroup* LinkerFindSectionGroup(Linker* linker, const char* name);
uint64_t LinkerSectionGroupSize(SectionGroup* group);
#endif /* linker_h */
