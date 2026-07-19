//
//  loader_arch_aarch64.c
//  c_compiler
//
//  Created by David Allison on 1/25/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "loader_arch_aarch64.h"
#include "elf.h"
#include <inttypes.h>
#include <string.h>

#define AARCH64_LR_REG 30
#define AARCH64_IP1_REG 16
#define AARCH64_IP2_REG 17

static uint64_t RuntimeAddress(LoadedDynamicLibrary* lib, uint64_t linked) {
  if (lib->loader != NULL && lib->loader->arch->ignore_vaddr) {
    uint64_t runtime = linked;
    LoaderLinkedAddressToRuntime(lib->loader, lib, linked, &runtime);
    return runtime;
  }
  return lib->load_address + linked;
}

static uint32_t EncodeAdrp(int rd, uint64_t pc, uint64_t target) {
  int64_t page_delta = ((int64_t)(target & ~0xfffULL) - (int64_t)(pc & ~0xfffULL)) >> 12;
  int32_t immlo = (int32_t)(page_delta & 3);
  int32_t immhi = (int32_t)((page_delta >> 2) & 0x7ffff);
  return (1u << 31) | ((uint32_t)immlo << 29) | (0x10u << 24) |
         ((uint32_t)immhi << 5) | (uint32_t)rd;
}

static uint32_t EncodeLdr64Imm(int rt, int rn, int32_t imm12) {
  return 0xF9400000u | ((uint32_t)(imm12 & 0xfff) << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rt;
}

static uint32_t EncodeSubReg(int rd, int rn, int rm) {
  return 0xCB000000u | ((uint32_t)rm << 16) | ((uint32_t)rn << 5) |
         (uint32_t)rd;
}

static uint32_t EncodeAddImm(int rd, int rn, int32_t imm12) {
  return 0x91000000u | ((uint32_t)(imm12 & 0xfff) << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

static uint32_t EncodeSubImm(int rd, int rn, int32_t imm12) {
  return 0xD1000000u | ((uint32_t)(imm12 & 0xfff) << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

static uint32_t EncodeLsrImm(int rd, int rn, int shift) {
  return 0xD345FC00u | ((uint32_t)(shift & 0x3f) << 16) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

static uint32_t EncodeBr(int rn) {
  return 0xD61F0000u | ((uint32_t)rn << 5);
}

static uint32_t EncodeBlImm(uint64_t pc, uint64_t target) {
  int64_t offset = (int64_t)(target - pc);
  int32_t imm26 = (int32_t)(offset >> 2);
  return 0x94000000u | ((uint32_t)imm26 & 0x03ffffffu);
}

static bool IsBl(uint32_t insn) {
  return (insn & 0xFC000000u) == 0x94000000u;
}

static uint64_t BlTarget(uint64_t pc, uint32_t insn) {
  int32_t imm26 = (int32_t)(insn & 0x03FFFFFFu);
  if ((imm26 & 0x02000000) != 0) {
    imm26 |= 0xFC000000;
  }
  return pc + ((int64_t)imm26 << 2);
}

static void RedirectCallsToPlt(Loader* loader, LoadedDynamicLibrary* lib,
                               uint64_t trampoline_runtime,
                               uint64_t target_runtime) {
  for (size_t r = 0; r < loader->regions.length; r++) {
    Region* region = loader->regions.value.p[r];
    if (region->owner != lib || region->segment == NULL ||
        (region->segment->flags & PF(x)) == 0 ||
        (region->segment->flags & PF(w)) != 0) {
      continue;
    }
    uint64_t base = (uint64_t)(uintptr_t)region->address;
    for (uint64_t addr = base; addr + 4 <= base + (uint64_t)region->length;
         addr += 4) {
      uint32_t* insn_ptr = (uint32_t*)(uintptr_t)addr;
      uint32_t insn = *insn_ptr;
      if (!IsBl(insn)) {
        continue;
      }
      if (BlTarget(addr, insn) != trampoline_runtime) {
        continue;
      }
      *insn_ptr = EncodeBlImm(addr, target_runtime);
    }
  }
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

static void FixupResolverPLTEntry(uint64_t plt_runtime, uint64_t got_runtime) {
  const int resolver_target = 18;
  uint32_t* p = (uint32_t*)(uintptr_t)plt_runtime;
  p[0] = EncodeAdrp(AARCH64_IP1_REG, plt_runtime, got_runtime);
  p[1] = EncodeSubReg(AARCH64_IP2_REG, AARCH64_LR_REG, AARCH64_IP2_REG);
  p[2] = EncodeLdr64Imm(resolver_target, AARCH64_IP1_REG,
                        (int32_t)((got_runtime & 0xfff) / 8));
  p[3] = EncodeSubImm(AARCH64_IP2_REG, AARCH64_IP2_REG, 32 + 12);
  p[4] = EncodeAddImm(0, AARCH64_IP1_REG, (int32_t)(got_runtime & 0xfff));
  p[5] = EncodeLsrImm(AARCH64_IP2_REG, AARCH64_IP2_REG, 1);
  p[6] = EncodeLdr64Imm(0, 0, 1);
  p[7] = EncodeBr(resolver_target);
}

static void FixupPLTTrampoline(uint64_t plt_runtime, uint64_t got_slot_runtime,
                              char* got_slot, bool lazy) {
  (void)got_slot;
  (void)lazy;
  uint32_t* p = (uint32_t*)(uintptr_t)plt_runtime;
  p[0] = EncodeAdrp(AARCH64_IP1_REG, plt_runtime, got_slot_runtime);
  p[1] = EncodeLdr64Imm(AARCH64_IP2_REG, AARCH64_IP1_REG,
                        (int32_t)((got_slot_runtime & 0xfff) / 8));
  p[2] = EncodeBr(AARCH64_IP2_REG);
  p[3] = 0xD503201Fu;  // nop
}

// Eager PLT binding uses one of two strategies:
//   Linux: standard adrp/ldr/br PLT stubs + runtime GOT (native hardware path).
//   Other hosts: rewrite bl@plt call sites to bl symbol (avoids PLT indirection;
//   required for the interpreter and for macOS where native cross-DSO bl/ret
//   through anonymously mapped ELF is unreliable under PAC/BTI).
static void BindPltEager(Loader* loader, LoadedDynamicLibrary* lib,
                         uint64_t trampoline_runtime, char* got_slot) {
#if defined(__linux__)
  (void)loader;
  (void)lib;
  (void)trampoline_runtime;
  (void)got_slot;
#else
  RedirectCallsToPlt(loader, lib, trampoline_runtime, *(uint64_t*)got_slot);
#endif
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

static void FixupPLTAfterLoad(Loader* loader, LoadedDynamicLibrary* lib,
                              const ELFRelocation* plt_relocations,
                              int64_t num_relocations, bool lazy) {
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
    uint64_t trampoline_runtime = plt_runtime + (2 + (uint64_t)i) * 16;
    FixupPLTTrampoline(trampoline_runtime, got_slot_runtime, got_slot, lazy);
    if (!lazy) {
      BindPltEager(loader, lib, trampoline_runtime, got_slot);
    }
  }
}

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  const void* pltgot = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  uint64_t* resolver_data = (uint64_t*)pltgot;
  resolver_data[0] = (uint64_t)(uintptr_t)data;
  resolver_data[1] = (uint64_t)(uintptr_t)lib;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  (void)lazy;
  switch (ELF_R_TYPE(reloc->info)) {
    case R_AARCH64_ABS64:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint64_t*)target_address =
            RuntimeAddress(lib, *(uint64_t*)target_address + symbol->value +
                           reloc->addend);
      }
      break;

    case R_AARCH64_GLOB_DAT:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint64_t*)target_address =
            RuntimeAddress(lib, symbol->value + reloc->addend);
      }
      break;
      
    case R_AARCH64_RELATIVE:
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
    case R_AARCH64_JUMP_SLOT:
      if (lazy) {
        *(uint64_t*)target_address =
            RuntimeAddress(lib, *(uint64_t*)target_address);
        } else {
          if (symbol == NULL) {
            LoaderError("Relocation refers on undefined symbol '%s'\n",
                        sym_name);
          } else {
            *(uint64_t*)target_address =
                RuntimeAddress(lib, symbol->value);
          }
        }
      break;
     default:
      abort();
  }
}


void AARCH64LoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_AARCH64;
  arch->platform = "aarch64";
  arch->ignore_vaddr = false;
  arch->tls_tcb_size = AARCH64_TLS_TCB_SIZE;
  arch->init_tls_tcb = NULL;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
  arch->fixup_plt_after_load = FixupPLTAfterLoad;
}

LoaderArchitecture* NewAARCH64LoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  AARCH64LoaderArchitectureInit(arch);
  return arch;
}
