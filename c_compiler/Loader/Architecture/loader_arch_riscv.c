//
//  loader_arch_riscv.c
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include "loader_arch_riscv.h"

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  // Find the DT_PLTGOT entry in the dynamic section.  We initialize
  // the resolver data in there.
  const void* pltgot = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  // The two values stored for RISC-V in the PLTGOT are:
  // GOT+0: address of resolver code.
  // GOT+8: address of LoadedDynamicLibrary
  uint64_t* resolver_data = (uint64_t*)pltgot;
  resolver_data[0] = (uint64_t)data;
  resolver_data[1] = (uint64_t)lib;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  switch (ELF_R_TYPE(reloc->info)) {
    case R_RISCV_64:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint64_t*)target_address = *(uint64_t*)target_address +
              lib->load_address +
              symbol->value +
              reloc->addend;
      }
      break;
      
    case R_RISCV_RELATIVE:
      // Relative to the library load address with no symbol.
      *(uint64_t*)target_address = *(uint64_t*)target_address +
                                   lib->load_address +
                                   reloc->addend;
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
    case R_RISCV_JUMP_SLOT:
      if (lazy) {
        // Lazy symbol resolution, GOT refers to PLT entry.
        *(uint64_t*)target_address += lib->load_address;
      } else {
        // Non-lazy resolution, replace GOT entry by the address
        // of the actual symbol.
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

void RISCVLoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_RISC_V;
  arch->platform = "risc-v";
  arch->ignore_vaddr = false;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
  arch->fixup_plt_after_load = NULL;
}

LoaderArchitecture* NewRISCVLoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  RISCVLoaderArchitectureInit(arch);
  return arch;
}
