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

struct LinkerSymbol;
struct LinkerFile;
struct DynamicSection;

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
// + for dynamic shared objects:
// 5. .plt
// 6. .got
// 7. .dynamic

#define LINKER_NUM_EXTRA_STATIC_SECTIONS 4
#define LINKER_NUM_EXTRA_DYNAMIC_SECTIONS (LINKER_NUM_EXTRA_STATIC_SECTIONS + 3)

#define LINKER_NUM_EXTRA_SECTIONS(linker) (linker->dso ? \
      LINKER_NUM_EXTRA_DYNAMIC_SECTIONS : LINKER_NUM_EXTRA_STATIC_SECTIONS)

// So after we read in the object files and know how many sections
// we can calculate the section addresses.

// On Mac OS X, the system reserves all addresses below 4GB.
// RISC-V only has 32-bit offsets from PC so we need to keep
// all symbols within 32 bits of the code.
#if defined(__APPLE__)
#define LINKER_CODE_SEGMENT_START_ADDRESS 0x400000000LL
#define LINKER_DATA_SEGMENT_START_ADDRESS 0x410000000LL
#define LINKER_SEGMENT_ALIGNMENT 0x10000000LL
#elif defined(__linux__)
#define LINKER_CODE_SEGMENT_START_ADDRESS 0x40000000LL
#define LINKER_DATA_SEGMENT_START_ADDRESS 0x41000000LL
#define LINKER_SEGMENT_ALIGNMENT 0x1000000LL
#else
#error "Unknown operating system"
#endif

#define LINKER_NUM_SEGMENTS 3
#define LINKER_SECTION_HEADER_OFFSET (sizeof(ELFHeader) + \
          LINKER_NUM_SEGMENTS * sizeof(ELFProgramHeader))


struct Segment;

typedef struct {
  String name;
  Vector components;
  struct Segment* segment;
  int32_t type;
  int64_t flags;
  int64_t alignment;
  int64_t address;
} SectionGroup;

SectionGroup* NewSectionGroup(const String* name, int32_t type, int64_t flags, int64_t alignment);
void SectionGroupDestruct(SectionGroup* group);
void SectionGroupDelete(SectionGroup* group);

#if 0

typedef struct {
  ELFReaderSection* section;
  ELFWriterSectionContents contents;
} LinkerOutputSectionContents;

// This is a section that will appear in the output.  It comprises of a set
// of input sections (from the object files) along with the location of their
// data.  This data might be located as a mapped address from the input
// file (if it hasn't been modified by the linker) or it might be a piece
// of memory allocated on the heap if the linker has performed relocations on it.
// The section is allocated an address inside the segment.
typedef struct {
  String* name;
  Vector contents;      // Vector of LinkerOutputSectionContents*.
  int64_t address;
  int32_t flags;
  int32_t type;
  int32_t alignment;      // Alignment for section (power of 2).
} Section;
#endif

// A segment.  This consists of a set of sections.
typedef struct Segment {
  Vector sections;      // Vector of SectionGroup*.
  int64_t address;
} Segment;

void SegmentInit(Segment* segment);
void SegmentDestruct(Segment* segment);

typedef struct Linker {
  Vector files;
  HashTable global_symbol_table;
  Vector section_groups;      // All SectionGroups from the input.
  Segment code_segment;
  Segment data_segment;
  uint64_t nobits_address;
  size_t nobit_size;
  Vector library_search_path;
  Vector static_libraries;
  Vector dynamic_libraries;
  int elf_machine_type;
  int elf_flags;
  bool dso;           // True if we are building a shared object.
  struct DynamicSection* dynamic_section;
} Linker;

void LinkerInit(Linker* linker);
void LinkerDestruct(Linker* linker);

bool LinkerReadObjectFile(Linker* linker, String* filename);
void LinkerLinkAllFiles(Linker* linker);
void LinkerWriteOutput(Linker* linker, FILE* fp);

struct LinkerFile* LinkerReadDynamicObject(Linker* linker, String* filename);

struct LinkerSymbol* LinkerFindSymbol(HashTable* symbol_table,
                               const char* name);

void LinkerInsertSymbol(HashTable* symbol_table,
                        struct LinkerSymbol* sym);

void LinkerAddLibrarySearchDir(Linker* linker, String* dir);
bool LinkerAddLibrary(Linker* linker, const char* name);
void LinkerAddStaticLibrary(Linker* linker, const char* name);
void LinkerAddDynamicLibrary(Linker* linker, const char* name);
bool LinkerFindSymbolInStaticLibraries(Linker* linker, const char* name,
                                 ARArchive** archive,
                                 ARFile** file);

void LinkerError(struct LinkerFile* file, const char* error, ...);
void VLinkerError(struct LinkerFile* file, const char* error, va_list ap);

void LinkerWarning(struct LinkerFile* file, const char* warn, const char* error, ...);
void VLinkerWarning(struct LinkerFile* file, const char* warn, const char* error, va_list ap);

#endif /* linker_h */
