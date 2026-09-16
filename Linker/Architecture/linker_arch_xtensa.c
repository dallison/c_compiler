//
//  linker_arch_xtensa.c
//  c_compiler
//

#include "linker_arch_xtensa.h"

#include <stdint.h>
#include <stdlib.h>

#include "xtensa_isa.h"

static int64_t CodeStartAddress(Linker* linker) {
  return 0x40080000LL + LinkerSectionHeaderOffset(linker);
}

static int64_t DataStartAddress(Linker* linker, int64_t code_start,
                                int64_t code_size) {
  (void)code_start;
  (void)code_size;
  return 0x3ffb0000LL + LinkerSectionHeaderOffset(linker);
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  dynamic->global_offset_table.num_resolver_data_entries = 0;
  dynamic->global_offset_table.entry_size = 4;
  dynamic->procedure_linkage_table.num_reserved_entries = 0;
  dynamic->procedure_linkage_table.entry_size = 0;
}

static void HandlePICRelocation(
    DynamicLinker* dynamic, LinkerSymbol* symbol, Relocation* reloc,
    int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
  (void)symbol;
  (void)reloc;
  (void)append_data_to_got;
  (void)append_func_to_got;
  (void)append_to_plt;
  (void)dynamic;
}

static void ApplyRelocation(Linker* linker, ObjectFile* file,
                            Relocation* reloc, LinkerSymbol* symbol,
                            char* target_address, uint64_t S, int64_t A) {
  (void)linker;
  uint64_t P = reloc->section->address + (uint64_t)reloc->offset;
  switch (reloc->type) {
    case R_XTENSA_NONE:
      return;
    case R_XTENSA_ASM_EXPAND:
    case R_XTENSA_ASM_SIMPLIFY:
      LinkerError(file,
                  "Xtensa assembler relaxation relocations are unsupported");
      return;
    case R_XTENSA_32:
      *(uint32_t*)target_address = (uint32_t)(S + A);
      return;
    case R_XTENSA_32_PCREL:
      *(uint32_t*)target_address = (uint32_t)(S + A - P);
      return;
    case R_XTENSA_SLOT0_OP:
      if (XtensaPatchSlot0((uint8_t*)target_address, (uint32_t)P,
                           (uint32_t)(S + A))) {
        return;
      }
      LinkerError(file,
                  "Xtensa instruction relocation for %s is out of range "
                  "(P=0x%llx, S+A=0x%llx)",
                  symbol ? symbol->name.value : "<section>",
                  (unsigned long long)P,
                  (unsigned long long)(S + A));
      return;
    default:
      LinkerError(file, "Unsupported Xtensa relocation %d", reloc->type);
      return;
  }
}

static void CheckOptions(Linker* linker) {
  if (!linker->fully_static) {
    LinkerError(NULL,
                "ESP32 Xtensa milestone supports static linking only");
  }
}

LinkerArchitecture* NewXtensaLinkerArchitecture(void) {
  LinkerArchitecture* arch = calloc(1, sizeof(*arch));
  arch->machine_type = ELF_MACHINE_TYPE_XTENSA;
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->init_dynamic_linker = InitDynamicLinker;
  arch->handle_pic_relocation = HandlePICRelocation;
  arch->apply_relocation = ApplyRelocation;
  arch->check_options = CheckOptions;
  return arch;
}
