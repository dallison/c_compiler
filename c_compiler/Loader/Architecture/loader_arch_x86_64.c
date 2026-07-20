//
//  loader_arch_x86_64.c
//  c_compiler
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "loader_arch_x86_64.h"
static void InitTlsTcb(void* tcb, uint64_t thread_pointer) {
  memcpy(tcb, &thread_pointer, sizeof(thread_pointer));
}

#include "elf.h"
#include <inttypes.h>

static uint64_t RuntimeAddress(LoadedDynamicLibrary* lib, uint64_t linked) {
  if (lib->loader != NULL && lib->loader->arch->ignore_vaddr) {
    uint64_t runtime = linked;
    LoaderLinkedAddressToRuntime(lib->loader, lib, linked, &runtime);
    return runtime;
  }
  return lib->load_address + linked;
}

static char* RelocationTarget(Loader* loader, LoadedDynamicLibrary* lib,
                              uint64_t linked_offset) {
  if (loader->arch->ignore_vaddr) {
    uint64_t runtime = 0;
    if (!LoaderLinkedAddressToRuntime(loader, lib, linked_offset, &runtime)) {
      return NULL;
    }
    return (char*)(uintptr_t)runtime;
  }
  return (char*)(uintptr_t)(lib->load_address + linked_offset);
}

static bool SectionLinkedAddress(LoadedDynamicLibrary* lib, const char* name,
                                 uint64_t* linked) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->addr == 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, name) != 0) {
      continue;
    }
    *linked = sh->addr;
    return true;
  }
  return false;
}

static void PatchRip32(uint8_t* insn, uint64_t target, uint64_t rip_next) {
  int32_t disp = (int32_t)(target - rip_next);
  memcpy(insn + 2, &disp, 4);
}

static void FixupResolverPLTEntry(uint64_t plt_runtime, uint64_t got_runtime) {
  uint8_t* p = (uint8_t*)(uintptr_t)plt_runtime;
  PatchRip32(p, got_runtime + 8, plt_runtime + 6);
  PatchRip32(p + 6, got_runtime + 16, plt_runtime + 12);
}

static void FixupPLTTrampoline(uint64_t plt_runtime, uint64_t got_slot_runtime,
                               uint64_t plt0_runtime) {
  uint8_t* p = (uint8_t*)(uintptr_t)plt_runtime;
  PatchRip32(p, got_slot_runtime, plt_runtime + 6);
  PatchRip32(p + 11, plt0_runtime, plt_runtime + 16);
}

static void FixupPLTAfterLoad(Loader* loader, LoadedDynamicLibrary* lib,
                              const ELFRelocation* plt_relocations,
                              int64_t num_relocations, bool lazy) {
  (void)lazy;
  if (plt_relocations == NULL || num_relocations == 0) {
    return;
  }
  uint64_t plt_linked = 0;
  if (!SectionLinkedAddress(lib, ".plt", &plt_linked)) {
    return;
  }
  uint64_t plt_runtime = 0;
  if (!LoaderLinkedAddressToRuntime(loader, lib, plt_linked, &plt_runtime)) {
    return;
  }
  const void* pltgot =
      DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  uint64_t got_runtime = (uint64_t)(uintptr_t)pltgot;
  FixupResolverPLTEntry(plt_runtime, got_runtime);

  for (int64_t i = 0; i < num_relocations; i++) {
    const ELFRelocation* reloc = &plt_relocations[i];
    char* got_slot = RelocationTarget(loader, lib, reloc->offset);
    if (got_slot == NULL) {
      continue;
    }
    uint64_t got_slot_runtime = (uint64_t)(uintptr_t)got_slot;
    uint64_t trampoline_runtime = plt_runtime + (1 + (uint64_t)i) * 16;
    FixupPLTTrampoline(trampoline_runtime, got_slot_runtime, plt_runtime);
  }
}

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  const void* pltgot =
      DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  uint64_t* resolver_data = (uint64_t*)pltgot;
  resolver_data[1] = (uint64_t)(uintptr_t)lib;
  resolver_data[2] = (uint64_t)(uintptr_t)data;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  (void)lazy;
  switch (ELF_R_TYPE(reloc->info)) {
    case R_X86_64_64:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n", sym_name);
      } else {
        *(uint64_t*)target_address = RuntimeAddress(
            lib, *(uint64_t*)target_address + symbol->value + reloc->addend);
      }
      break;

    case R_X86_64_GLOB_DAT:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n", sym_name);
      } else {
        *(uint64_t*)target_address =
            RuntimeAddress(lib, symbol->value + reloc->addend);
      }
      break;

    case R_X86_64_RELATIVE:
      *(uint64_t*)target_address =
          RuntimeAddress(lib, *(uint64_t*)target_address + reloc->addend);
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
    case R_X86_64_JUMP_SLOT:
      if (lazy) {
        *(uint64_t*)target_address =
            RuntimeAddress(lib, *(uint64_t*)target_address);
      } else {
        if (symbol == NULL) {
          LoaderError("Relocation refers on undefined symbol '%s'\n", sym_name);
        } else {
          *(uint64_t*)target_address = RuntimeAddress(lib, symbol->value);
        }
      }
      break;

    default:
      abort();
  }
}

void X86_64LoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_X86_64;
  arch->platform = "x86_64";
  // The software interpreter executes using host addresses, so segments must
  // be mapped at their linked virtual addresses (host == vaddr).  This keeps
  // RIP-relative references across the text and data segments valid.
  arch->ignore_vaddr = false;
  arch->tls_tcb_size = X86_64_TLS_TP_SLOT_SIZE;
  arch->init_tls_tcb = InitTlsTcb;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
  arch->fixup_plt_after_load = FixupPLTAfterLoad;
}

LoaderArchitecture* NewX86_64LoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  X86_64LoaderArchitectureInit(arch);
  return arch;
}
