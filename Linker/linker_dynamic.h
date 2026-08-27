//
//  linker_dynamic.h
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

//
// This is the dynamic support for the linker.  It supports reading
// and writing dynamic libraries and executables as neede by
// the linker.  It is not the runtime dynamic loader.
//
#ifndef linker_dynamic_h
#define linker_dynamic_h

#include "vector.h"
#include <stdint.h>
#include "loader_dynamic.h"

struct LinkerSymbol;
struct Linker;
struct ELFWriterSectionContents;
struct DynamicLinker;

// Dynamic Shared Object (DSO)

// A DSO can be loaded at runtime to provide shared libraries.
// Each .so file contains:
// 1. A symbol table containing the exported and imported symbols in
//    the library
// 2. A Global Offset Table (GOT) consisting of the addresses of every
//    external variable used or defined by the library.  This is held
//    in read/write memory and the addresses are set when the file is loaded.
// 3. A Procedure Linkage Table (PLT) containing trampolines for each
//    function called inside the library.  The PLT entries are set to the
//    real address of the function upon load or when the procedure is first
//    called.
// 4. Relocations to lookup the addresses of the symbols in the GOT and
//    assign them to the entry.


// There are a number of parts to the GOT.  The first part contains the
// addresses of data and will be relocated at load time to contain the
// correct address.  This is used for access to global variables referenced
// from a DSO.
//
// The second part contains the offsets of TLS variabled that uss the
// InitialExec model.  These are offsets into the TLS data allocated for
// every thread.
//
// The third part is pairs of words for variables using the TLS Global Dynamic
// model.  The first word is the module id and the second is the offset
// into the TLS data for that module.

// The fourth part of the GOT is for the addresses of functions.  When loaded
// these are all set to an address that allows the dynamic linker to lookup
// the address of the function and set the GOT entry when the function
// is first called.
//
// The first three parts of the GOT are held in the section ".got" and are
// relocated at load time to contain the absolute addresses of variables or
// TLS offsets.
//
// The fourth part of the GOT is held in the section ".got.plt" and at load
// time contains the addresses of the PLT entries so that the resolution
// of symbols can be done lazyily on the first call to the function.
typedef struct {
  int num_resolver_data_entries;  // Number of entries for resolver data.
  int entry_size;          // Size of each entry (4 or 8).
  Vector data_entries;     // Vector of pointers to LinkerSymbol.
  Vector tls_ie_entries;      // Vector of pointers to LinkerSymbol.
  Vector tls_gd_entries;      // Vector of pointers to LinkerSymbol.
  Vector function_entries; // Vector of pointers to LinkerSymbol.
} GlobalOffsetTable;

// The PLT consists of a set of trampolines that relay a call
// to its correct address, after first locating the symbol
// using dynamic lookup at runtime.
typedef struct {
  int num_reserved_entries;       // Reserved for runtime resolver.
  int entry_size;                 // Size of each entry.
  Vector trampolines;             // Vector of LinkerSymbol*.
} ProcedureLinkageTable;

typedef struct DynamicLinker {
  GlobalOffsetTable global_offset_table;
  ProcedureLinkageTable procedure_linkage_table;
  Vector got_relocations;
  Vector plt_relocations;
  Vector data_relocations;      // Pointers to Relocation.
  Vector needed_libraries;      // Offsets into dynstr table.
  Vector dynamic_symbol_fixups; // Private dynamic-symbol build records.
  ELF_Xword rpath;              // Offset into dynstr table.
  struct LinkerSymbol* global_offset_table_symbol;
  struct LinkerSymbol* dynamic_symbol;
  
  // These are the synthetic sections created by the dynamic
  // linker.  The linker always deals with groups of sections
  // at a time so these groups have a single section in them.
  struct SectionGroup* got_group;
  struct SectionGroup* got_plt_group;
  struct SectionGroup* plt_group;
  struct SectionGroup* dyn_rela_group;
  struct SectionGroup* plt_rela_group;
  struct SectionGroup* data_rela_group;
  struct SectionGroup* dynamic_group;
  struct SectionGroup* interpreter_group;
  
  // All loaded libraries.
  DynamicLibraryRegistry loaded_dynamic_libraries;
} DynamicLinker;

void DynamicLinkerInit(DynamicLinker* s, struct Linker* linker);
DynamicLinker* NewDynamicLinker(struct Linker* linker);
void DynamicLinkerDestruct(DynamicLinker* s);
void DynamicLinkerDelete(DynamicLinker* s);
void DynamicLinkerInventSymbols(struct Linker* linker, DynamicLinker* s);
void DynamicLinkerDefineSymbols(Linker* linker);

void DynamicLinkerBuildDynamicRelocations(struct Linker* linker);
void DynamicLinkerBuildPLTRelocations(struct Linker* linker);
void DynamicLinkerGatherDynamicRelocations(struct Linker* linker);
void DynamicLinkerCreateDynamicLinkerGroups(Linker* linker);
void DynamicLinkerFixupDynamicSymbolTable(struct Linker* linker,
                                          Buffer* dynsym,
                                          int32_t bss_section_index);
void DynamicLinkerFixupDynamicSectionContents(ELFWriterFile* elf);
void DynamicLinkerFixupPLT(Linker* linker);
void DynamicLinkerFixupGOT(Linker* linker);

// Position-independent code in a fully static executable still reads variable
// addresses out of the GOT, but there is no loader to fill the slots in.  When
// DynamicLinkerNeedsStaticGOT reports that such a link collected GOT entries,
// the table is created and named like any other section group and then given
// its final contents once every address is known.
bool DynamicLinkerNeedsStaticGOT(Linker* linker);
void DynamicLinkerCreateStaticGOTGroups(Linker* linker);
void DynamicLinkerDefineStaticGOTSymbol(Linker* linker);
void DynamicLinkerResolveStaticGOT(Linker* linker);


#endif /* linker_dynamic_h */
