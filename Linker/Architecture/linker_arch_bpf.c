#include "linker_arch_bpf.h"

#include <stdlib.h>
#include <string.h>
#include "elf.h"

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    address = LINKER_DSO_CODE_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker) +
              1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address = LINKER_CODE_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker);
  } else {
    address = LINKER_CODE_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker) +
              2 * linker->ops->program_header_size;
  }
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
             linker->ops->section_header_size;
  return address;
}

static int64_t DataStartAddress(Linker* linker, int64_t code_start,
                                int64_t code_size) {
  int64_t address;
  if (linker->building_dso) {
    address = (code_start + LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1) &
              ~(LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1);
    address += LinkerSectionHeaderOffset(linker) +
               1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address = LINKER_DATA_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker);
  } else {
    address = LINKER_DATA_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker) +
              2 * linker->ops->program_header_size;
  }
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
                 linker->ops->section_header_size +
             code_size;
  return address;
}

static void PatchLddw(char* target, uint64_t value) {
  uint32_t lo = (uint32_t)value;
  uint32_t hi = (uint32_t)(value >> 32);
  memcpy(target + 4, &lo, sizeof(lo));
  memcpy(target + 12, &hi, sizeof(hi));
}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  (void)linker;
  uint64_t P = reloc->section->address + reloc->offset;
  switch (reloc->type) {
    case R_BPF_64_64:
      PatchLddw(target_address, S + (uint64_t)A);
      break;
    case R_BPF_64_ABS64:
      memcpy(target_address, &(uint64_t){S + (uint64_t)A}, 8);
      break;
    case R_BPF_64_ABS32:
    case R_BPF_64_NODYLD32: {
      uint32_t v = (uint32_t)(S + (uint64_t)A);
      memcpy(target_address, &v, 4);
      break;
    }
    case R_BPF_64_32: {
      int64_t delta = (int64_t)(S + (uint64_t)A) - (int64_t)P - 8;
      int32_t imm = (int32_t)(delta / 8);
      memcpy(target_address + 4, &imm, 4);
      break;
    }
    default:
      LinkerError(file, "Unsupported BPF relocation type %d", reloc->type);
      break;
  }
}

static void HandlePICRelocation(DynamicLinker* dynamic, LinkerSymbol* symbol,
                                Relocation* reloc,
                                int (*append_data_to_got)(DynamicLinker*,
                                                          LinkerSymbol*),
                                int (*append_func_to_got)(DynamicLinker*,
                                                          LinkerSymbol*),
                                int (*append_to_plt)(DynamicLinker*,
                                                     LinkerSymbol*)) {
  (void)dynamic;
  (void)symbol;
  (void)reloc;
  (void)append_data_to_got;
  (void)append_func_to_got;
  (void)append_to_plt;
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  dynamic->global_offset_table.num_resolver_data_entries = 0;
  dynamic->global_offset_table.entry_size = 8;
  dynamic->procedure_linkage_table.num_reserved_entries = 0;
  dynamic->procedure_linkage_table.entry_size = 8;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents, Vector* relocs,
                        GOTRelocation relocation_type) {
  (void)linker;
  (void)symbol;
  (void)contents;
  (void)relocs;
  (void)relocation_type;
}

static void FixupGOTEntry(Linker* linker, LinkerSymbol* symbol,
                          Buffer* got_plt_buffer, uint64_t plt_address,
                          int plt_entry_size) {
  (void)linker;
  (void)symbol;
  (void)got_plt_buffer;
  (void)plt_address;
  (void)plt_entry_size;
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  (void)linker;
  (void)symbol;
  (void)contents;
}

static void SetupResolverPLTEntry(Linker* linker, ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {
  (void)linker;
  (void)plt;
  (void)plt_buffer;
  (void)got_address;
  (void)plt_address;
}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {
  (void)plt;
  (void)got;
  (void)symbol;
  (void)plt_buffer;
  (void)got_address;
  (void)plt_address;
}

static void CheckOptions(Linker* linker) { (void)linker; }

LinkerArchitecture* NewBPFLinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->machine_type = ELF_MACHINE_TYPE_BPF;
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->handle_pic_relocation = HandlePICRelocation;
  arch->apply_relocation = ApplyRelocation;
  arch->init_dynamic_linker = InitDynamicLinker;
  arch->add_got_entry = AddGOTEntry;
  arch->fixup_got_entry = FixupGOTEntry;
  arch->add_plt_entry = AddPLTEntry;
  arch->setup_resolver_plt_entry = SetupResolverPLTEntry;
  arch->fixup_plt_entry = FixupPLTEntry;
  arch->check_options = CheckOptions;
  return arch;
}
