//
//  linker_arch_aarch64.c
//  c_compiler
//
//  Created by David Allison on 1/25/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#include "linker_arch_aarch64.h"

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    // We have an extra segment when build a dynamic object (the DYNAMIC
    // segment).
    address = LINKER_DSO_CODE_SEGMENT_START_ADDRESS +
              LINKER_SECTION_HEADER_OFFSET + 1 * sizeof(ELFProgramHeader);
  } else if (linker->fully_static) {
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET;
  } else {
    // Dynamic executable, 2 extra segments: INTERP and DYNAMIC.
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET +
              2 * sizeof(ELFProgramHeader);
  }
  // We know how many sections there are now.  This is the number of groups +
  // the number of extra sections we add.  Add space for the section headers,
  // each of which is sizeof(ELFSectionHeader) bytes long. We also are going to
  // create a BSS section.
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
             sizeof(ELFSectionHeader);
  return address;
}

static int64_t DataStartAddress(Linker* linker, int64_t code_start,
                                int64_t code_size) {
  int64_t address = code_start;
  if (linker->building_dso) {
    // For a DSO, the data segment is after the code segment but aligned to
    // the next boundary specified by
    // LINKER_DSO_DATA_SEGMENT_START_ADDRESS_ALIGNMENT
    address = (address + LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1) &
              ~(LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1);
    address += LINKER_SECTION_HEADER_OFFSET + 1 * sizeof(ELFProgramHeader);
  } else if (linker->fully_static) {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET;
  } else {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LINKER_SECTION_HEADER_OFFSET +
              2 * sizeof(ELFProgramHeader);
  }
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
                 sizeof(ELFSectionHeader) +
             code_size;
  return address;
}

// Find the relocation associated with pc.
static Relocation* FindRelocation(Linker* linker, int64_t pc) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    ObjectFile* file = linker->files.value.p[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      Relocation* reloc = file->relocations.value.p[reloc_index];
      int64_t target_address = reloc->section->address + reloc->offset;
      if (target_address == pc) {
        return reloc;
      }
    }
  }
  return NULL;
}

static void HandlePICRelocation(
    DynamicLinker* dynamic, LinkerSymbol* symbol, Relocation* reloc,
    int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t P = reloc->section->address + reloc->offset;
  int32_t hi21;
  int32_t lo12;

  // NOTE: a break in this switch will result in a linker error due to an
  // unsupported relocation.  To support a relocation, use return, not break.
  switch (reloc->type) {
    case R_AARCH64_NONE:
      break;

    //  ILP32
    case R_AARCH64_P32_ABS32:
      break;

    case R_AARCH64_P32_COPY:
      break;

    case R_AARCH64_P32_GLOB_DAT:
      break;

    case R_AARCH64_P32_JUMP_SLOT:
      break;

    case R_AARCH64_P32_RELATIVE:
      break;

    case R_AARCH64_P32_TLS_DTPMOD:
      break;

    case R_AARCH64_P32_TLS_DTPREL:
      break;

    case R_AARCH64_P32_TLS_TPREL:
      break;

    case R_AARCH64_P32_TLSDESC:
      break;

    case R_AARCH64_P32_IRELATIVE:
      break;

    // LP64 AArch64
    case R_AARCH64_ABS64:
      *((int64_t*)target_address) = (int64_t)(S + A);
      return;

    case R_AARCH64_ABS32:
      *((int32_t*)target_address) = (int32_t)(S + A);
      return;

    case R_AARCH64_ABS16:
      *((int16_t*)target_address) = (int16_t)(S + A);
      return;

    case R_AARCH64_PREL64:
      *((int64_t*)target_address) = (int64_t)(S - P - A);
      return;

    case R_AARCH64_PREL32:
      *((int32_t*)target_address) = (int32_t)(S - P - A);
      return;

    case R_AARCH64_PREL16:
      *((int16_t*)target_address) = (int16_t)(S - P - A);
      return;

    case R_AARCH64_MOVW_UABS_G0:
      break;

    case R_AARCH64_MOVW_UABS_G0_NC:
      break;

    case R_AARCH64_MOVW_UABS_G1:
      break;

    case R_AARCH64_MOVW_UABS_G1_NC:
      break;

    case R_AARCH64_MOVW_UABS_G2:
      break;

    case R_AARCH64_MOVW_UABS_G2_NC:
      break;

    case R_AARCH64_MOVW_UABS_G3:
      break;

    case R_AARCH64_MOVW_SABS_G0:
      break;

    case R_AARCH64_MOVW_SABS_G1:
      break;

    case R_AARCH64_MOVW_SABS_G2:
      break;

    case R_AARCH64_LD_PREL_LO19:
      break;

    case R_AARCH64_ADR_PREL_LO21: {
      int64_t offset = (int64_t)(S + A - P);
      uint32_t instruction = *(uint32_t*)target_address;
      hi21 = (int32_t)((offset >> 2) & 0x7ffff);
      lo12 = (int32_t)(offset & 0x3);
      instruction &= ~((0x3u << 29) | (0x7ffffu << 5));
      instruction |= (lo12 << 29) | (hi21 << 5);
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_ADR_PREL_PG_HI21:
      break;

    case R_AARCH64_ADR_PREL_PG_HI21_NC:
      break;

    case R_AARCH64_ADD_ABS_LO12_NC:
      break;

    case R_AARCH64_LDST8_ABS_LO12_NC:
      break;

    case R_AARCH64_TSTBR14:
      break;

    case R_AARCH64_CONDBR19:
      break;

    case R_AARCH64_JUMP26: {
      int64_t offset = (int64_t)(S + A - P);
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~0x03ffffffu;
      instruction |= (uint32_t)((offset >> 2) & 0x03ffffff);
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_CALL26: {
      int64_t offset = (int64_t)(S + A - P);
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~0x03ffffffu;
      instruction |= (uint32_t)((offset >> 2) & 0x03ffffff);
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_LDST16_ABS_LO12_NC:
      break;

    case R_AARCH64_LDST32_ABS_LO12_NC:
      break;

    case R_AARCH64_LDST64_ABS_LO12_NC:
      break;

    case R_AARCH64_MOVW_PREL_G0:
      break;

    case R_AARCH64_MOVW_PREL_G0_NC:
      break;

    case R_AARCH64_MOVW_PREL_G1:
      break;

    case R_AARCH64_MOVW_PREL_G1_NC:
      break;

    case R_AARCH64_MOVW_PREL_G2:
      break;

    case R_AARCH64_MOVW_PREL_G2_NC:
      break;

    case R_AARCH64_MOVW_PREL_G3:
      break;

    case R_AARCH64_LDST128_ABS_LO12_NC:
      break;

    case R_AARCH64_MOVW_GOTOFF_G0:
      break;

    case R_AARCH64_MOVW_GOTOFF_G0_NC:
      break;

    case R_AARCH64_MOVW_GOTOFF_G1:
      break;

    case R_AARCH64_MOVW_GOTOFF_G1_NC:
      break;

    case R_AARCH64_MOVW_GOTOFF_G2:
      break;

    case R_AARCH64_MOVW_GOTOFF_G2_NC:
      break;

    case R_AARCH64_MOVW_GOTOFF_G3:
      break;

    case R_AARCH64_GOTREL64:
      break;

    case R_AARCH64_GOTREL32:
      break;

    case R_AARCH64_GOT_LD_PREL19:
      break;

    case R_AARCH64_LD64_GOTOFF_LO15:
      break;

    case R_AARCH64_ADR_GOT_PAGE:
      break;

    case R_AARCH64_LD64_GOT_LO12_NC:
      break;

    case R_AARCH64_LD64_GOTPAGE_LO15:
      break;

    case R_AARCH64_TLSGD_ADR_PREL21:
      break;

    case R_AARCH64_TLSGD_ADR_PAGE21:
      break;

    case R_AARCH64_TLSGD_ADD_LO12_NC:
      break;

    case R_AARCH64_TLSGD_MOVW_G1:
      break;

    case R_AARCH64_TLSGD_MOVW_G0_NC:
      break;

    case R_AARCH64_TLSLD_ADR_PREL21:
      break;

    case R_AARCH64_TLSLD_ADR_PAGE21:
      break;

    case R_AARCH64_TLSLD_ADD_LO12_NC:
      break;

    case R_AARCH64_TLSLD_MOVW_G1:
      break;

    case R_AARCH64_TLSLD_MOVW_G0_NC:
      break;

    case R_AARCH64_TLSLD_LD_PREL19:
      break;

    case R_AARCH64_TLSLD_MOVW_DTPREL_G2:
      break;

    case R_AARCH64_TLSLD_MOVW_DTPREL_G1:
      break;

    case R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC:
      break;

    case R_AARCH64_TLSLD_MOVW_DTPREL_G0:
      break;

    case R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC:
      break;

    case R_AARCH64_TLSLD_ADD_DTPREL_HI12:
      break;

    case R_AARCH64_TLSLD_ADD_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLD_LDST8_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLD_LDST16_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLD_LDST32_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLD_LDST64_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSIE_MOVW_GOTTPREL_G1:
      break;

    case R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC:
      break;

    case R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21:
      break;

    case R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC:
      break;

    case R_AARCH64_TLSIE_LD_GOTTPREL_PREL19:
      break;

    case R_AARCH64_TLSLE_MOVW_TPREL_G2:
      break;

    case R_AARCH64_TLSLE_MOVW_TPREL_G1:
      break;

    case R_AARCH64_TLSLE_MOVW_TPREL_G1_NC:
      break;

    case R_AARCH64_TLSLE_MOVW_TPREL_G0:
      break;

    case R_AARCH64_TLSLE_MOVW_TPREL_G0_NC:
      break;

    case R_AARCH64_TLSLE_ADD_TPREL_HI12:
      break;

    case R_AARCH64_TLSLE_ADD_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_ADD_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLE_LDST8_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLE_LDST16_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLE_LDST32_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLE_LDST64_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSDESC_LD_PREL19:
      break;

    case R_AARCH64_TLSDESC_ADR_PREL21:
      break;

    case R_AARCH64_TLSDESC_ADR_PAGE21:
      break;

    case R_AARCH64_TLSDESC_LD64_LO12:
      break;

    case R_AARCH64_TLSDESC_ADD_LO12:
      break;

    case R_AARCH64_TLSDESC_OFF_G1:
      break;

    case R_AARCH64_TLSDESC_OFF_G0_NC:
      break;

    case R_AARCH64_TLSDESC_LDR:
      break;

    case R_AARCH64_TLSDESC_ADD:
      break;

    case R_AARCH64_TLSDESC_CALL:
      break;

    case R_AARCH64_TLSLE_LDST128_TPREL_LO12:
      break;

    case R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC:
      break;

    case R_AARCH64_TLSLD_LDST128_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC:
      break;

    case R_AARCH64_COPY:
      break;

    case R_AARCH64_GLOB_DAT:
      break;

    case R_AARCH64_JUMP_SLOT:
      break;

    case R_AARCH64_RELATIVE:
      break;

    case R_AARCH64_TLS_DTPMOD:
      break;

    case R_AARCH64_TLS_DTPREL:
      break;

    case R_AARCH64_TLS_TPREL:
      break;

    case R_AARCH64_TLSDESC:
      break;

    case R_AARCH64_IRELATIVE:
      break;
  }
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  // 2 reserved entries at the beginning of the PLTGOT:
  // 0: address of runtime resolver.
  // 1: pointer to data structure for dynamic loader
  dynamic->global_offset_table.num_resolver_data_entries = 2;
  dynamic->global_offset_table.entry_size = 8;

  // First entry of PLT contains jump to the runtime resolver.
  dynamic->procedure_linkage_table.num_reserved_entries = 2;
  dynamic->procedure_linkage_table.entry_size = 16;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents, Vector* relocs,
                        GOTRelocation relocation_type) {
  int64_t offset = contents->data.buffered.length;
  int32_t reloc_type;
  switch (relocation_type) {
    case kGOTRelocationFunction:
      reloc_type = R_RISCV_JUMP_SLOT;
      break;
    case kGOTRelocationVariable:
      reloc_type = R_RISCV_64;
      break;
    case kGOTRelocationTLSOffset:
      reloc_type = R_RISCV_TLS_DTPREL64;
      break;
    case kGOTRelocationTLSModuleId:
      reloc_type = R_RISCV_TLS_DTPMOD64;
      break;
  }
  BufferAppendLongLE(&contents->data.buffered, 0);

  // Add relocation.
  Relocation* reloc = NewLinkerSymbolRelocation(symbol, offset, reloc_type, 0);
  VectorAppend(relocs, reloc);
}

// The GOT entry in the .got.plt is set to the address of the plt.
static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  // The GOT entry points to the first entry in the PLT, which contains
  // the symbol resolver code.
  uint64_t* p = (uint64_t*)got_plt_buffer->value + symbol->got_index;
  *p = plt_address;
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {}

static void CheckOptions(Linker* linker) {}

LinkerArchitecture* NewAARCH64LinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->machine_type = ELF_MACHINE_TYPE_AARCH64;
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
