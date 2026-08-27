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
              LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
  } else {
    // Dynamic executable, 2 extra segments: INTERP and DYNAMIC.
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker) +
              2 * linker->ops->program_header_size;
  }
  // We know how many sections there are now.  This is the number of groups +
  // the number of extra sections we add.  Add space for the section headers,
  // each of which is linker->ops->section_header_size bytes long. We also are going to
  // create a BSS section.
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
             linker->ops->section_header_size;
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
    address += LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
  } else {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker) +
              2 * linker->ops->program_header_size;
  }
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
                 linker->ops->section_header_size +
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

static COMPILER_UNUSED uint32_t EncodeBlr(int rn) {
  return 0xD63F0000u | ((uint32_t)rn << 5);
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

static void PatchImmediate12(char* target_address, uint64_t value,
                             unsigned scale) {
  uint32_t instruction = *(uint32_t*)target_address;
  instruction &= ~(0xfffu << 10);
  instruction |= (uint32_t)((value >> scale) & 0xfff) << 10;
  *(uint32_t*)target_address = instruction;
}

static void PatchMoveWideImmediate(char* target_address, uint64_t value,
                                   unsigned shift) {
  uint32_t instruction = *(uint32_t*)target_address;
  instruction &= ~(0xffffu << 5);
  instruction |= (uint32_t)((value >> shift) & 0xffff) << 5;
  *(uint32_t*)target_address = instruction;
}

static void HandlePICRelocation(
    DynamicLinker* dynamic, LinkerSymbol* symbol, Relocation* reloc,
    int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
  switch (reloc->type) {
    case R_AARCH64_ADR_GOT_PAGE:
      if (symbol != NULL) {
        symbol->got_index = append_data_to_got(dynamic, symbol);
      }
      break;

    case R_AARCH64_CALL_PLT:
      if (symbol != NULL) {
        symbol->got_index = append_func_to_got(dynamic, symbol);
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;

    case R_AARCH64_ABS64: {
      Relocation* rel_reloc = NewDataAddressRelocation(
          symbol, reloc, R_AARCH64_RELATIVE, R_AARCH64_GLOB_DAT);
      VectorAppend(&dynamic->data_relocations, rel_reloc);
      break;
    }
  }
}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t P = reloc->section->address + reloc->offset;
  int32_t hi21;
  int32_t lo12;
  uint64_t tprel = S + A + AARCH64_TLS_TCB_SIZE;

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
      *((int64_t*)target_address) = (int64_t)(S + A - P);
      return;

    case R_AARCH64_PREL32:
      *((int32_t*)target_address) = (int32_t)(S + A - P);
      return;

    case R_AARCH64_PREL16:
      *((int16_t*)target_address) = (int16_t)(S + A - P);
      return;

    case R_AARCH64_TLSLE_MOVW_TPREL_G2:
      PatchMoveWideImmediate(target_address, tprel, 32);
      return;

    case R_AARCH64_TLSLE_MOVW_TPREL_G1:
    case R_AARCH64_TLSLE_MOVW_TPREL_G1_NC:
      PatchMoveWideImmediate(target_address, tprel, 16);
      return;

    case R_AARCH64_TLSLE_MOVW_TPREL_G0:
    case R_AARCH64_TLSLE_MOVW_TPREL_G0_NC:
      PatchMoveWideImmediate(target_address, tprel, 0);
      return;

    case R_AARCH64_TLSLE_ADD_TPREL_HI12:
      PatchImmediate12(target_address, tprel, 12);
      return;

    case R_AARCH64_TLSLE_ADD_TPREL_LO12:
    case R_AARCH64_TLSLE_ADD_TPREL_LO12_NC:
    case R_AARCH64_TLSLE_LDST8_TPREL_LO12:
    case R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC:
      PatchImmediate12(target_address, tprel, 0);
      return;

    case R_AARCH64_TLSLE_LDST16_TPREL_LO12:
    case R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC:
      PatchImmediate12(target_address, tprel, 1);
      return;

    case R_AARCH64_TLSLE_LDST32_TPREL_LO12:
    case R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC:
      PatchImmediate12(target_address, tprel, 2);
      return;

    case R_AARCH64_TLSLE_LDST64_TPREL_LO12:
    case R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC:
      PatchImmediate12(target_address, tprel, 3);
      return;

    case R_AARCH64_TLSLE_LDST128_TPREL_LO12:
    case R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC:
      PatchImmediate12(target_address, tprel, 4);
      return;

    case R_AARCH64_TLS_TPREL:
      *(uint64_t*)target_address = tprel;
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

    case R_AARCH64_ADR_PREL_PG_HI21: {
      uint64_t got_address = 0;
      uint64_t addr = S + A;
      if (symbol != NULL && symbol->got_index >= 0 &&
          linker->dynamic_linker != NULL &&
          linker->dynamic_linker->got_plt_group != NULL) {
        got_address = linker->dynamic_linker->got_plt_group->address;
        addr = got_address + (uint64_t)symbol->got_index * 8 + (uint64_t)A;
      }
      uint32_t instruction = EncodeAdrp(0, P, addr);
      instruction = (instruction & ~0x1fu) | (*(uint32_t*)target_address & 0x1fu);
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_ADR_PREL_PG_HI21_NC:
      break;

    case R_AARCH64_ADD_ABS_LO12_NC: {
      uint64_t value = (uint64_t)(S + A);
      uint32_t imm12 = (uint32_t)(value & 0xfff);
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~(0xfffu << 10);
      instruction |= imm12 << 10;
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_LDST8_ABS_LO12_NC:
    case R_AARCH64_LDST16_ABS_LO12_NC:
    case R_AARCH64_LDST32_ABS_LO12_NC:
    case R_AARCH64_LDST64_ABS_LO12_NC:
    case R_AARCH64_LDST128_ABS_LO12_NC: {
      unsigned scale = reloc->type == R_AARCH64_LDST128_ABS_LO12_NC ? 4
                       : reloc->type == R_AARCH64_LDST64_ABS_LO12_NC ? 3
                       : reloc->type == R_AARCH64_LDST32_ABS_LO12_NC ? 2
                       : reloc->type == R_AARCH64_LDST16_ABS_LO12_NC ? 1
                                                                    : 0;
      uint32_t imm12 = (uint32_t)(((S + A) & 0xfff) >> scale);
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~(0xfffu << 10);
      instruction |= imm12 << 10;
      *(uint32_t*)target_address = instruction;
      return;
    }

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

    case R_AARCH64_CALL_PLT: {
      // A fully static link builds no PLT because there is no runtime resolver
      // for a trampoline to reach; the callee's address is final, so the call
      // goes straight to it.
      uint64_t addr;
      if (linker->dynamic_linker == NULL || symbol == NULL ||
          symbol->plt_index < 0 ||
          linker->dynamic_linker->plt_group == NULL) {
        addr = S + (uint64_t)A;
      } else {
        addr = linker->dynamic_linker->plt_group->address +
               (uint64_t)symbol->plt_index * 16 + (uint64_t)A;
      }
      int64_t offset = (int64_t)(addr - P);
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~0x03ffffffu;
      instruction |= (uint32_t)((offset >> 2) & 0x03ffffff);
      *(uint32_t*)target_address = instruction;
      return;
    }

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

    case R_AARCH64_ADR_GOT_PAGE: {
      if (linker->dynamic_linker->got_plt_group == NULL) {
        LinkerError(file, "GOT relocation in a link with no GOT");
        return;
      }
      uint64_t got_address = linker->dynamic_linker->got_plt_group->address;
      uint64_t addr = got_address + (uint64_t)symbol->got_index * 8 + (uint64_t)A;
      uint32_t instruction = EncodeAdrp(0, P, addr);
      instruction = (instruction & ~0x1fu) | (*(uint32_t*)target_address & 0x1fu);
      *(uint32_t*)target_address = instruction;
      return;
    }

    case R_AARCH64_LD64_GOT_LO12_NC: {
      Relocation* page_reloc = FindRelocation(linker, (int64_t)(P - 4));
      if (page_reloc == NULL ||
          page_reloc->type != R_AARCH64_ADR_GOT_PAGE) {
        LinkerError(file, "Missing ADR_GOT_PAGE for LD64_GOT_LO12_NC");
        return;
      }
      if (linker->dynamic_linker->got_plt_group == NULL) {
        LinkerError(file, "GOT relocation in a link with no GOT");
        return;
      }
      LinkerSymbol* got_symbol =
          ObjectFileFindSymbol(file, page_reloc->symbol_name.value);
      uint64_t got_address = linker->dynamic_linker->got_plt_group->address;
      uint64_t addr = got_address + (uint64_t)got_symbol->got_index * 8 +
                      (uint64_t)page_reloc->addend;
      uint32_t instruction = *(uint32_t*)target_address;
      instruction &= ~0x003ffc00u;
      instruction |= (uint32_t)((addr & 0xfff) << 10);
      *(uint32_t*)target_address = instruction;
      return;
    }

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

    case R_AARCH64_TLSLD_LDST128_DTPREL_LO12:
      break;

    case R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC:
      break;

    case R_AARCH64_COPY:
      break;

    case R_AARCH64_GLOB_DAT:
      *((int64_t*)target_address) = (int64_t)(S + A);
      return;

    case R_AARCH64_JUMP_SLOT:
      return;

    case R_AARCH64_RELATIVE:
      return;

    case R_AARCH64_TLS_DTPMOD:
      break;

    case R_AARCH64_TLS_DTPREL:
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
      reloc_type = R_AARCH64_JUMP_SLOT;
      break;
    case kGOTRelocationVariable:
      reloc_type = R_AARCH64_ABS64;
      break;
    case kGOTRelocationTLSOffset:
      reloc_type = R_AARCH64_TLS_DTPREL;
      break;
    case kGOTRelocationTLSModuleId:
      reloc_type = R_AARCH64_TLS_DTPMOD;
      break;
  }
  BufferAppendLongLE(&contents->data.buffered, 0);

  // Add relocation.
  Relocation* reloc = NewLinkerSymbolRelocation(symbol, offset, reloc_type, 0);
  VectorAppend(relocs, reloc);
}

// The GOT entry points to the PLT resolver for lazy resolution.
static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  (void)symbol;
  (void)plt_entry_size;
  uint64_t* p = (uint64_t*)got_plt_buffer->value + symbol->got_index;
  *p = plt_address;
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  (void)linker;
  (void)symbol;
  BufferAppendWordLE(&contents->data.buffered,
                     EncodeAdrp(AARCH64_IP1_REG, 0, 0));
  BufferAppendWordLE(&contents->data.buffered,
                     EncodeLdr64Imm(AARCH64_IP2_REG, AARCH64_IP1_REG, 0));
  BufferAppendWordLE(&contents->data.buffered, EncodeBr(AARCH64_IP2_REG));
  BufferAppendWordLE(&contents->data.buffered, 0xD503201Fu);  // nop
}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {
  (void)plt;
  const int resolver_target = 18;

  uint32_t* p = (uint32_t*)plt_buffer->value;
  p[0] = EncodeAdrp(AARCH64_IP1_REG, plt_address, got_address);
  p[1] = EncodeSubReg(AARCH64_IP2_REG, AARCH64_LR_REG, AARCH64_IP2_REG);
  p[2] = EncodeLdr64Imm(resolver_target, AARCH64_IP1_REG,
                         (int32_t)((got_address & 0xfff) / 8));
  p[3] = EncodeSubImm(AARCH64_IP2_REG, AARCH64_IP2_REG, 32 + 12);
  p[4] = EncodeAddImm(0, AARCH64_IP1_REG, (int32_t)(got_address & 0xfff));
  p[5] = EncodeLsrImm(AARCH64_IP2_REG, AARCH64_IP2_REG, 1);
  p[6] = EncodeLdr64Imm(0, 0, 1);
  p[7] = EncodeBr(resolver_target);
}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {
  uint64_t offset = (uint64_t)symbol->plt_index * (uint64_t)plt->entry_size;
  uint64_t entry_address =
      got_address + (uint64_t)symbol->got_index * (uint64_t)got->entry_size;
  uint64_t trampoline_address = plt_address + offset;
  uint32_t* p = (uint32_t*)(plt_buffer->value + offset);

  p[0] = EncodeAdrp(AARCH64_IP1_REG, trampoline_address, entry_address);
  p[1] = EncodeLdr64Imm(AARCH64_IP2_REG, AARCH64_IP1_REG,
                         (int32_t)((entry_address & 0xfff) / 8));
}

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
