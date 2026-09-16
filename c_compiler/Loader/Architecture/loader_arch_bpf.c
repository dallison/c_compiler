#include "loader_arch_bpf.h"

#include <stdlib.h>
#include "elf.h"

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  (void)lib;
  (void)data;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name, char* target_address,
                                   bool lazy) {
  (void)lib;
  (void)reloc;
  (void)symbol;
  (void)sym_name;
  (void)target_address;
  (void)lazy;
}

static void ApplyGOTPLTRelocation(LoadedDynamicLibrary* lib,
                                  const ELFRelocation* reloc,
                                  const ELFSymbol* symbol, const char* sym_name,
                                  char* target_address, bool lazy) {
  (void)lib;
  (void)reloc;
  (void)symbol;
  (void)sym_name;
  (void)target_address;
  (void)lazy;
}

void BPFLoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_BPF;
  arch->platform = "bpf";
  arch->ignore_vaddr = false;
  arch->tls_tcb_size = 0;
  arch->init_tls_tcb = NULL;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
  arch->fixup_plt_after_load = NULL;
}

LoaderArchitecture* NewBPFLoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  BPFLoaderArchitectureInit(arch);
  return arch;
}
