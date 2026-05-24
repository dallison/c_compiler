//
//  loader_arch_arm.c
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "loader_arch_arm.h"
#include "elf.h"
#include <unistd.h>

static int64_t SegmentFileOffsetDelta(const ELFProgramHeader* segment) {
  int64_t page_size = sysconf(_SC_PAGESIZE);
  int64_t page_mask = page_size - 1;
  return (int64_t)segment->offset - (segment->offset & ~page_mask);
}

static uint32_t EncodeBlImm(uint64_t pc, uint64_t target) {
  int64_t offset = (int64_t)(target - pc) - 8;
  int32_t imm24 = (int32_t)(offset >> 2);
  return 0xEB000000u | ((uint32_t)imm24 & 0x00ffffffu);
}

static bool IsBl(uint32_t insn) {
  return (insn & 0x0e000000u) == 0x0a000000u && (insn & 0x01000000u) != 0;
}

static uint64_t BlTarget(uint64_t pc, uint32_t insn) {
  int32_t imm24 = (int32_t)(insn & 0x00ffffffu);
  if ((imm24 & 0x00800000) != 0) {
    imm24 |= 0xFF000000;
  }
  return pc + 8 + ((int64_t)imm24 << 2);
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

static void FixupResolverPLTEntry(uint64_t plt_runtime, uint64_t got_runtime) {
  uint32_t* p = (uint32_t*)(uintptr_t)plt_runtime;
  p[6] = (uint32_t)(got_runtime - (plt_runtime + 16));
}

static void FixupPLTTrampoline(uint64_t trampoline_runtime,
                               uint64_t got_slot_runtime) {
  uint32_t* p = (uint32_t*)(uintptr_t)trampoline_runtime;
  p[3] = (uint32_t)(got_slot_runtime - (trampoline_runtime + 12));
}

static void RedirectCallsToPlt(Loader* loader, LoadedDynamicLibrary* lib,
                               uint64_t trampoline_runtime,
                               uint64_t target_linked) {
  uint64_t target_runtime = target_linked;
  if (!LoaderLinkedAddressToRuntime(loader, NULL, target_linked,
                                    &target_runtime)) {
    return;
  }
  for (size_t r = 0; r < loader->regions.length; r++) {
    Region* region = loader->regions.value.p[r];
    if (region->segment == NULL ||
        (region->segment->flags & PF(x)) == 0 ||
        (region->segment->flags & PF(w)) != 0) {
      continue;
    }
    if (region->owner != lib &&
        !(region->owner == NULL && lib == loader->dynamic_lib)) {
      continue;
    }
    uint64_t base = (uint64_t)(uintptr_t)region->address +
                    (uint64_t)SegmentFileOffsetDelta(region->segment);
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

static void BindPltEager(Loader* loader, LoadedDynamicLibrary* lib,
                         uint64_t trampoline_runtime, uint32_t target_linked) {
  (void)loader;
  (void)lib;
  (void)trampoline_runtime;
  (void)target_linked;
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
    FixupPLTTrampoline(trampoline_runtime, got_slot_runtime);
    if (!lazy) {
      BindPltEager(loader, lib, trampoline_runtime,
                   *(uint32_t*)got_slot);
    }
  }
}

static void InitGOTPLT(LoadedDynamicLibrary* lib, void* data) {
  // Find the DT_PLTGOT entry in the dynamic section.  We initialize
  // the resolver data in there.
  const void* pltgot = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(pltgot));
  if (pltgot == NULL) {
    return;
  }
  // ARM EABI: two 32-bit words at the start of .got.plt:
  // GOT+0: address of resolver code (linked; patched at runtime for lazy).
  // GOT+4: address of LoadedDynamicLibrary.
  uint32_t* resolver_data = (uint32_t*)pltgot;
  resolver_data[0] = (uint32_t)(uintptr_t)data;
  resolver_data[1] = (uint32_t)(uintptr_t)lib;
}

static void ApplyGOTDataRelocation(LoadedDynamicLibrary* lib,
                                   const ELFRelocation* reloc,
                                   const ELFSymbol* symbol,
                                   const char* sym_name,
                                   char* target_address,
                                   bool lazy) {
  (void)lazy;
  switch (ELF_R_TYPE(reloc->info)) {
    case R_ARM_ABS32:
      if (symbol == NULL) {
        LoaderError("Relocation refers on undefined symbol '%s'\n",
                    sym_name);
      } else {
        *(uint32_t*)target_address =
            (uint32_t)(*(uint32_t*)target_address + symbol->value +
                       (uint32_t)reloc->addend);
      }
      break;

    case R_ARM_RELATIVE:
      *(uint32_t*)target_address =
          (uint32_t)(*(uint32_t*)target_address + (uint32_t)reloc->addend);
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
  (void)reloc;
  (void)sym_name;
  switch (ELF_R_TYPE(reloc->info)) {
    case R_ARM_JUMP_SLOT:
      if (lazy) {
        // Keep the link-time GOT contents (PLT entry linked address).
        (void)lib;
        (void)target_address;
        } else {
          if (symbol == NULL) {
            LoaderError("Relocation refers on undefined symbol '%s'\n",
                        sym_name);
          } else {
            // Store the linked symbol address; the interpreter translates it.
            *(uint32_t*)target_address = (uint32_t)symbol->value;
          }
        }
      break;

    default:
      abort();
  }
}

void ARMLoaderArchitectureInit(LoaderArchitecture* arch) {
  arch->machine_type = ELF_MACHINE_TYPE_ARM;
  arch->platform = "arm";
  arch->ignore_vaddr = true;
  arch->init_got_plt = InitGOTPLT;
  arch->apply_got_data_relocation = ApplyGOTDataRelocation;
  arch->apply_got_plt_relocation = ApplyGOTPLTRelocation;
  arch->fixup_plt_after_load = FixupPLTAfterLoad;
}

LoaderArchitecture* NewARMLoaderArchitecture(void) {
  LoaderArchitecture* arch = malloc(sizeof(LoaderArchitecture));
  ARMLoaderArchitectureInit(arch);
  return arch;
}
