//
//  loader_arch_pcode.c
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include "loader_arch_pcode.h"

// Data is the address of the symbol resolver code.
static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  // Find the DT_PLTGOT entry in the dynamic section.  We initialize
  // the resolver data in there.
  const void* pltgot = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  // The two values stored for P-Code in the PLTGOT are:
  // GOT+0: address of LoadedDynamicLibrary
  // GOT+8: address of resolver code.
  uint64_t* resolver_data = (uint64_t*)pltgot;
  resolver_data[0] = (uint64_t)lib;
  resolver_data[1] = (uint64_t)data;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  switch (ELF_R_TYPE(reloc->info)) {
    case R_PCODE_GOT_DATA:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint64_t*)target_address = lib->load_address + symbol->value;
      }
      break;
    case R_PCODE_GOT_FUNC:
      LoaderError("Unexpected GOT_FUNC relocation in .rela.dyn");
      break;
    default:
      abort();
  }
}

static void ApplyGOTPLTRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  switch (ELF_R_TYPE(reloc->info)) {
    case R_PCODE_GOT_DATA:
      LoaderError("Unexpected GOT_DATA relocation in .rela.dyn");
      break;
    case R_PCODE_GOT_FUNC:
      if (lazy) {
        // Lazy symbol resolution, GOT refers to PLT entry.
        *(uint64_t*)target_address += lib->load_address;
      } else {
        // Non-lazy resolution, replace GOT entry by the address
        // of the symbol.
        if (symbol == NULL) {
          LoaderError("Relocation refers on undefined symbol '%s'\n",
                      sym_name);
        } else {
          *(uint64_t*)target_address = lib->load_address + symbol->value;
        }
      }
      break;
    default:
      abort();
  }
  
}

void PCodeLoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_PCODE;
  arch->platform = "p-code";
  arch->ignore_vaddr = false;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
}

LoaderArchitecture* NewPCodeLoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  PCodeLoaderArchitectureInit(arch);
  return arch;
}
