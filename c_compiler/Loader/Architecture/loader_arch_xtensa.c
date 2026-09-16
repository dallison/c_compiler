//
//  loader_arch_xtensa.c
//  c_compiler
//

#include "loader_arch_xtensa.h"

#include <stdint.h>
#include <stdlib.h>

#include "elf.h"

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  (void)lib;
  (void)data;
}

static void ApplyDynamicRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* symbol_name,
                                   char* target_address, bool lazy) {
  (void)lazy;
  uint32_t type = ELF_R_TYPE(reloc->info);
  if (type == R_XTENSA_RELATIVE) {
    *(uint32_t*)target_address =
        (uint32_t)(lib->load_address + reloc->addend);
  } else if ((type == R_XTENSA_32 || type == R_XTENSA_GLOB_DAT ||
              type == R_XTENSA_JMP_SLOT) &&
             symbol != NULL) {
    *(uint32_t*)target_address =
        (uint32_t)(lib->load_address + symbol->value + reloc->addend);
  } else {
    LoaderError("unsupported Xtensa dynamic relocation %u for %s\n", type,
                symbol_name == NULL ? "<none>" : symbol_name);
  }
}

void XtensaLoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_XTENSA;
  arch->platform = "xtensa-esp32";
  // Keep 32-bit linked addresses in guest registers and translate them on each
  // memory access.  This works on 64-bit hosts without requiring fixed mmap
  // addresses in the ESP32 range.
  arch->ignore_vaddr = true;
  arch->tls_tcb_size = 0;
  arch->init_tls_tcb = NULL;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyDynamicRelocation;
  arch->apply_got_plt_relocation = ApplyDynamicRelocation;
  arch->fixup_plt_after_load = NULL;
}

LoaderArchitecture* NewXtensaLoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(*arch));
  XtensaLoaderArchitectureInit(arch);
  return arch;
}
