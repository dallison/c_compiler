//
//  arch.h
//  p_code_linker
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "linker.h"
#include "linker_file.h"
#include "linker_symbols.h"
#include "linker_reloc.h"
#include "linker_dynamic.h"

#ifndef arch_h
#define arch_h

typedef enum {
  kGOTRelocationVariable,
  kGOTRelocationFunction,
  kGOTRelocationTLSOffset,
  kGOTRelocationTLSModuleId,
} GOTRelocation;

// This struct holds pointer to functions that perform the architecture
// specific aspects of the linker.
typedef struct LinkerArchitecture {
  int machine_type;
  
  int64_t (*code_start_address)(Linker* linker);
  int64_t (*data_start_address)(Linker* linker, int64_t code_start,
                                int64_t code_size);
  
  // Initialize the dynamic linker data structures.
  void (*init_dynamic_linker)(DynamicLinker* dynamic);
  
  // We have a relocation in the .o file.  If it's a PIC relocation
  // add the GOT and PLT entries.
  void (*handle_pic_relocation)(DynamicLinker* dynamic,
                                LinkerSymbol* symbol,
                                Relocation* reloc,
                                int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
                                int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
                               int (*append_to_plt)(DynamicLinker*, LinkerSymbol*));
  
  // Apply a relocation to the given address.
  void (*apply_relocation)(Linker* linker,
                           ObjectFile* file,
                           Relocation* reloc,
                           LinkerSymbol* symbol,
                           char* target_address,
                           uint64_t S, int64_t A);
  
  // Add an entry to the GOT section data for the given symbol.
  void (*add_got_entry)(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
                        GOTRelocation relocation_type);
  
  // Fixup the GOT entry for the symbol now that addresses are known.
  void (*fixup_got_entry)(Linker* linker, LinkerSymbol* symbol,
                          Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size);
  
  // Add entry to the PLT section for the given symbol.
  void (*add_plt_entry)(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents);
  
  // Setup the resolver entry in the PLT.  This is the first entry
  // and calls the runtile symbol resolver for lazy symbol
  // resolution.
  void (*setup_resolver_plt_entry)(Linker* linker, ProcedureLinkageTable* plt,
                                   Buffer* plt_buffer,
                                   uint64_t got_address,
                                   uint64_t plt_address);
  
  // Fixup a PLT entry now that addresses are known.
  void (*fixup_plt_entry)(ProcedureLinkageTable* plt,
                          GlobalOffsetTable* got,
                          LinkerSymbol* symbol,
                          Buffer* plt_buffer,
                          uint64_t got_address,
                          uint64_t plt_address);
  
  void (*check_options)(Linker* linker);
  
} LinkerArchitecture;

#endif /* arch_h */
