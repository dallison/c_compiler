//
//  linker_arch_arm.c
//  p_code_linker
//
//  Created by David Allison on 3/7/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include "linker_arch_arm.h"

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    address = LINKER_DATA_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker) +
              1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address =
        LINKER_CODE_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
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
  int64_t address = code_start;
  if (linker->building_dso) {
    address = (address + LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1) &
              ~(LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1);
    address += LinkerSectionHeaderOffset(linker) +
               1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address =
        LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
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

static uint32_t EncodeLdrFromPc(int rd, int32_t offset) {
  return ARM_AL | 0x059f0000u | ((uint32_t)rd << 12) |
         ((uint32_t)offset & 0xfffu);
}

static void SetBitField32(char* target_address, int lsb, int width,
                          int32_t value) {
  int32_t mask = (1 << width) - 1;
  value &= mask;

  mask <<= lsb;
  int32_t word = *(int32_t*)target_address;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

static void AddBitField32(char* target_address, int lsb, int width,
                          int32_t value) {
  int32_t mask = (1 << width) - 1;

  int32_t word = *(int32_t*)target_address;
  int32_t old_value = (word >> lsb) & mask;

  value += old_value;
  value &= mask;

  mask <<= lsb;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

static void SubBitField32(char* target_address, int lsb, int width,
                          int32_t value) {
  int32_t mask = (1 << width) - 1;

  int32_t word = *(int32_t*)target_address;
  int32_t old_value = (word >> lsb) & mask;

  value = old_value - value;
  value &= mask;

  mask <<= lsb;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

static void SetBranchOffset(char* target_address, int64_t offset_bytes) {
  int32_t offset_words = (int32_t)(offset_bytes >> 2);
  SetBitField32(target_address, 0, 24, offset_words);
}

static void SetMovwMovtImm16(char* target_address, uint32_t imm16) {
  uint32_t word = *(uint32_t*)target_address;
  uint32_t imm4 = (imm16 >> 12) & 0xf;   // high 4 bits -> bits[19:16]
  uint32_t imm12 = imm16 & 0xfffu;       // low 12 bits -> bits[11:0]
  word &= ~0x000f0fffu;
  word |= (imm4 << 16) | imm12;
  *(uint32_t*)target_address = word;
}

static uint64_t GOTOriginAddress(Linker* linker) {
  if (linker->dynamic_linker == NULL) {
    return 0;
  }
  if (linker->dynamic_linker->global_offset_table_symbol != NULL) {
    return linker->dynamic_linker->global_offset_table_symbol->address;
  }
  if (linker->dynamic_linker->got_plt_group != NULL) {
    return linker->dynamic_linker->got_plt_group->address;
  }
  return 0;
}

static uint64_t GOTEntryAddress(Linker* linker, LinkerSymbol* symbol) {
  if (linker->dynamic_linker == NULL || symbol == NULL ||
      symbol->got_index < 0) {
    return 0;
  }
  if (symbol->plt_index >= 0 &&
      linker->dynamic_linker->got_plt_group != NULL) {
    return linker->dynamic_linker->got_plt_group->address +
           (uint64_t)symbol->got_index *
               linker->dynamic_linker->global_offset_table.entry_size;
  }
  if (linker->dynamic_linker->got_group != NULL) {
    return linker->dynamic_linker->got_group->address +
           (uint64_t)symbol->got_index *
               linker->dynamic_linker->global_offset_table.entry_size;
  }
  return 0;
}

static uint64_t PLTEntryAddress(Linker* linker, LinkerSymbol* symbol,
                                int64_t addend) {
  if (linker->dynamic_linker == NULL || symbol == NULL ||
      symbol->plt_index < 0 ||
      linker->dynamic_linker->plt_group == NULL) {
    return 0;
  }
  return linker->dynamic_linker->plt_group->address +
         (uint64_t)symbol->plt_index *
             linker->dynamic_linker->procedure_linkage_table.entry_size +
         (uint64_t)addend;
}

static void HandlePICRelocation(
    DynamicLinker* dynamic, LinkerSymbol* symbol, Relocation* reloc,
    int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
  switch (reloc->type) {
    case R_ARM_GOT_BREL:
      if (symbol != NULL) {
        symbol->got_index = append_data_to_got(dynamic, symbol);
      }
      break;

    case R_ARM_PLT32:
      if (symbol != NULL) {
        symbol->got_index = append_func_to_got(dynamic, symbol);
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;

    case R_ARM_CALL:
      if (symbol != NULL && symbol->plt_index < 0) {
        symbol->got_index = append_func_to_got(dynamic, symbol);
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;

    case R_ARM_ABS32: {
      Relocation* rel_reloc = NewRelativeRelocation(
          reloc->offset, reloc->section, R_ARM_RELATIVE, reloc->addend);
      VectorAppend(&dynamic->data_relocations, rel_reloc);
      break;
    }
  }
}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t P = reloc->section->address + reloc->offset;

  // NOTE: a break in this switch will result in a linker error due to an
  // unsupported relocation.  To support a relocation, use return, not break.
  switch (reloc->type) {
    case R_ARM_NONE:
      return;

    case R_ARM_ABS32:
    case R_ARM_TARGET1:
      *(int32_t*)target_address = (int32_t)(S + A);
      return;

    case R_ARM_REL32:
      *(int32_t*)target_address = (int32_t)(S + A - P);
      return;

    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24: {
      int64_t target = (int64_t)(S + A);
      if (symbol != NULL && symbol->plt_index >= 0) {
        target = (int64_t)PLTEntryAddress(linker, symbol, A);
      }
      SetBranchOffset(target_address, target - ((int64_t)P + 8));
      return;
    }

    case R_ARM_MOVW_ABS_NC:
      SetMovwMovtImm16(target_address, (uint32_t)(S + A) & 0xffffu);
      return;

    case R_ARM_MOVT_ABS:
      SetMovwMovtImm16(target_address, (uint32_t)((S + A) >> 16) & 0xffffu);
      return;

    case R_ARM_MOVW_PREL_NC: {
      int64_t value = (int64_t)(S + A - P);
      SetMovwMovtImm16(target_address, (uint32_t)value & 0xffffu);
      return;
    }

    case R_ARM_MOVT_PREL: {
      int64_t value = (int64_t)(S + A - P);
      SetMovwMovtImm16(target_address, (uint32_t)(value >> 16) & 0xffffu);
      return;
    }

    case R_ARM_GLOB_DAT:
      *(int32_t*)target_address = (int32_t)(S + A);
      return;

    case R_ARM_JUMP_SLOT:
    case R_ARM_RELATIVE:
    case R_ARM_COPY:
      return;

    case R_ARM_GOT_BREL: {
      uint64_t gotorg = GOTOriginAddress(linker);
      uint64_t got_entry = GOTEntryAddress(linker, symbol);
      *(int32_t*)target_address = (int32_t)(got_entry + (uint64_t)A - gotorg);
      return;
    }

    case R_ARM_PLT32: {
      uint64_t B = PLTEntryAddress(linker, symbol, A);
      if (B == 0) {
        B = S + (uint64_t)A;
      }
      int64_t offset = (int64_t)(B - (P + 8));
      uint32_t word = *(uint32_t*)target_address;
      if ((word & 0x0e000000u) == 0x0a000000u) {
        SetBranchOffset(target_address, offset);
      } else {
        *(int32_t*)target_address = (int32_t)offset;
      }
      return;
    }

    case R_ARM_ADD8:
      AddBitField32(target_address, 0, 8, (int32_t)(S + A));
      return;

    case R_ARM_ADD16:
      AddBitField32(target_address, 0, 16, (int32_t)(S + A));
      return;

    case R_ARM_ADD32:
      *(int32_t*)target_address += (int32_t)(S + A);
      return;

    case R_ARM_SUB8:
      SubBitField32(target_address, 0, 8, (int32_t)(S + A));
      return;

    case R_ARM_SUB16:
      SubBitField32(target_address, 0, 16, (int32_t)(S + A));
      return;

    case R_ARM_SUB32:
      *(int32_t*)target_address -= (int32_t)(S + A);
      return;

    default:
      break;
  }
  LinkerError(file, "Unsupported ARM relocation type %d", reloc->type);
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  // 2 reserved entries at the beginning of the PLT:
  // PLT0 resolver stub and its literal pool word.
  dynamic->global_offset_table.num_resolver_data_entries = 2;
  dynamic->global_offset_table.entry_size = 4;

  dynamic->procedure_linkage_table.num_reserved_entries = 2;
  dynamic->procedure_linkage_table.entry_size = 16;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents, Vector* relocs,
                        GOTRelocation relocation_type) {
  (void)linker;
  int32_t reloc_type;
  switch (relocation_type) {
    case kGOTRelocationFunction:
      reloc_type = R_ARM_JUMP_SLOT;
      break;
    case kGOTRelocationVariable:
      reloc_type = R_ARM_ABS32;
      break;
    case kGOTRelocationTLSOffset:
    case kGOTRelocationTLSModuleId:
      reloc_type = R_ARM_ABS32;
      break;
  }
  int64_t offset = contents->data.buffered.length;
  BufferAppendWordLE(&contents->data.buffered, 0);

  Relocation* reloc = NewLinkerSymbolRelocation(symbol, offset, reloc_type, 0);
  VectorAppend(relocs, reloc);
}

static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  (void)plt_entry_size;
  (void)symbol;
  // Lazy binding: GOT entry initially points to the PLT resolver stub.
  // Store the full linked PLT address; the dynamic linker applies relocs.
  uint32_t* p = (uint32_t*)got_plt_buffer->value + symbol->got_index;
  *p = (uint32_t)(plt_address & 0xffffffffu);
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  (void)linker;
  (void)symbol;
  BufferAppendWordLE(&contents->data.buffered,
                     EncodeLdrFromPc(ARM_IP_REG, 4));
  BufferAppendWordLE(&contents->data.buffered, 0xe08fc00cu);  // add ip, pc, ip
  BufferAppendWordLE(&contents->data.buffered, 0xe51cf000u);  // ldr pc, [ip]
  BufferAppendWordLE(&contents->data.buffered, 0);
}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {
  (void)plt;
  uint32_t* p = (uint32_t*)plt_buffer->value;

  // First reserved 16-byte slot: lazy resolver trampoline (no stack push;
  // the interpreter resolves via SWI without using the stack as a link chain).
  p[0] = 0xe1a00000u;  // nop
  p[1] = EncodeLdrFromPc(ARM_IP_REG, 12);
  p[2] = 0xe08fc00cu;  // add ip, pc, ip
  p[3] = 0xe51cf000u;  // ldr pc, [ip]

  // Second reserved 16-byte slot: GOT[0] offset literal and padding.
  p[4] = 0xe1a00000u;  // nop
  p[5] = 0xe1a00000u;  // nop
  p[6] = (uint32_t)(got_address - (plt_address + 16));
  p[7] = 0xe1a00000u;  // nop
}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {
  uint64_t offset =
      (uint64_t)symbol->plt_index * (uint64_t)plt->entry_size;
  uint64_t entry_address =
      got_address + (uint64_t)symbol->got_index * (uint64_t)got->entry_size;
  uint64_t trampoline_address = plt_address + offset;
  uint32_t* p = (uint32_t*)(plt_buffer->value + offset);

  p[3] = (uint32_t)(entry_address - (trampoline_address + 12));
}

static void CheckOptions(Linker* linker) {
  (void)linker;
}

LinkerArchitecture* NewARMLinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->machine_type = ELF_MACHINE_TYPE_ARM;
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
