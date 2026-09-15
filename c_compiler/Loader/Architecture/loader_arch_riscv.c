//
//  loader_arch_riscv.c
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include "elf.h"
#include "loader.h"
#include "loader_arch_riscv.h"

static bool LibIsELF32(const LoadedDynamicLibrary* lib) {
  if (lib != NULL && lib->loader != NULL && lib->loader->elf_file != NULL &&
      lib->loader->elf_file->ops != NULL) {
    return !lib->loader->elf_file->ops->is_64_bit;
  }
  return false;
}

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  // Find the DT_PLTGOT entry in the dynamic section.  We initialize
  // the resolver data in there.
  const void* pltgot = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  if (LibIsELF32(lib)) {
    // Host pointers cannot be represented in an RV32 GOT.  Lazy dynamic
    // binding is therefore unavailable to the in-process interpreter.
    uint32_t* resolver_data = (uint32_t*)pltgot;
    resolver_data[0] = 0;
    resolver_data[1] = 0;
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
    case R_RISCV_32:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint32_t*)target_address = (uint32_t)(
            (LibIsELF32(lib) ? 0 : lib->load_address) + symbol->value +
            reloc->addend);
      }
      break;
    case R_RISCV_64:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        // As for R_RISCV_RELATIVE below, this comes from SHT_RELA and the
        // addend in the entry is the whole contribution; the place holds the
        // link-time value so that a native ld.so agrees, and adding it here
        // would count the addend twice.
        *(uint64_t*)target_address =
            lib->load_address + symbol->value + reloc->addend;
      }
      break;
      
    case R_RISCV_RELATIVE:
      // Relative to the library load address with no symbol.  These come from
      // SHT_RELA, where the addend alone is the link-time value; the place
      // contributes nothing.  The linker also leaves that value in the place so
      // a native ld.so and this loader agree, so adding the two would double
      // the address.
      if (LibIsELF32(lib)) {
        *(uint32_t*)target_address = (uint32_t)reloc->addend;
      } else {
        *(uint64_t*)target_address = lib->load_address + reloc->addend;
      }
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
      if (LibIsELF32(lib)) {
        if (lazy) {
          // RV32 guest pointers remain linked virtual addresses.  The
          // interpreter translates them when accessing host memory.
        } else if (symbol == NULL) {
          LoaderError("Relocation refers on undefined symbol '%s'\n",
                      sym_name);
        } else {
          *(uint32_t*)target_address = (uint32_t)symbol->value;
        }
        break;
      }
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
  arch->tls_tcb_size = RISCV_TLS_TCB_SIZE;
  arch->init_tls_tcb = NULL;
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
