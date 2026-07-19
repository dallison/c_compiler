//
//  linker_arch_riscv.c
//  p_code_linker
//
//  Created by David Allison on 3/7/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include "linker_arch_riscv.h"

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    // We have an extra segment when build a dynamic object (the DYNAMIC
    // segment).
    address =  LINKER_DSO_CODE_SEGMENT_START_ADDRESS +
    LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address =  LINKER_CODE_SEGMENT_START_ADDRESS +
    LinkerSectionHeaderOffset(linker);
  } else {
    // Dynamic executable, 2 extra segments: INTERP and DYNAMIC.
    address =  LINKER_CODE_SEGMENT_START_ADDRESS +
    LinkerSectionHeaderOffset(linker) + 2 * linker->ops->program_header_size;
  }
  // We know how many sections there are now.  This is the number of groups + the number
  // of extra sections we add.  Add space for the section headers, each of which is
  // linker->ops->section_header_size bytes long.
  // We also are going to create a BSS section.
  address += (linker->section_groups.length +
              LINKER_NUM_EXTRA_SECTIONS + 1) *
  linker->ops->section_header_size;
  return address;
}

static int64_t DataStartAddress(Linker* linker, int64_t code_start, int64_t code_size) {
  int64_t address = code_start;
  if (linker->building_dso) {
    // For a DSO, the data segment is after the code segment but aligned to
    // the next boundary specified by
    // LINKER_DSO_DATA_SEGMENT_START_ADDRESS_ALIGNMENT
    address = (address +
               LINKER_DYNAMIC_SEGMENT_ALIGNMENT - 1) &
    ~(LINKER_DYNAMIC_SEGMENT_ALIGNMENT-1);
    address += LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
  } else {
    // For a static executable the data segment has a fixed address.
    address = LINKER_DATA_SEGMENT_START_ADDRESS +
    LinkerSectionHeaderOffset(linker) +
    2 * linker->ops->program_header_size;
  }
  address += (linker->section_groups.length +
              LINKER_NUM_EXTRA_SECTIONS + 1) *
      linker->ops->section_header_size + code_size;
  return address;
}

static void HandlePICRelocation(DynamicLinker* dynamic, LinkerSymbol* symbol,
                                Relocation* reloc,
                                int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
                                int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
                                int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
  switch (reloc->type) {
    case R_RISCV_GOT_HI20:
    case R_RISCV_TLS_GOT_HI20:
      if (symbol != NULL) {
        symbol->got_index = append_data_to_got(dynamic, symbol);
      }
      break;
 
    case R_RISCV_TLS_GD_HI20:
      if (symbol != NULL) {
        // This needs two GOT entries.
        symbol->got_index = append_data_to_got(dynamic, symbol);
        append_data_to_got(dynamic, symbol);
        symbol->got_index--;    // Back to first entry.
      }
      break;

    case R_RISCV_CALL_PLT:
      if (symbol != NULL) {
        // Add GOT entry for function address.
        symbol->got_index = append_func_to_got(dynamic, symbol);
        
        // Add PLT entry for call to GOT.
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;
      
    case R_RISCV_64: {
      // This is used for a relocation to a local symbol.
      // Build a RELATIVE relocation and add it to the data_relocations
      // in the dynamic linker.
      Relocation* rel_reloc = NewRelativeRelocation(reloc->offset,
                                                    reloc->section,
                                                    R_RISCV_RELATIVE,
                                                    reloc->addend);
      VectorAppend(&dynamic->data_relocations, rel_reloc);
      break;
    }
  }
}

// Set a bit field in a 32 bit word at the target address.  The bits in the word
// are overwritten.
static void SetBitField32(char* target_address, int lsb, int width, int32_t value) {
  int32_t mask = (1 << width) - 1;
  value &= mask;
  
  mask <<= lsb;
  int32_t word = *(int32_t*)target_address;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

// Adds a bit field in a 32 bit word at the target address.
static void AddBitField32(char* target_address, int lsb, int width, int32_t value) {
  int32_t mask = (1 << width) - 1;
  
  // Extract old value from word in lower bits.
  int32_t word = *(int32_t*)target_address;
  int32_t old_value = (word >> lsb) & mask;
  
  // Add in value and mask with width.
  value += old_value;
  value &= mask;
  
  // Or in the new value at the bitfield position.
  mask <<= lsb;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

// Adds a bit field in a 32 bit word at the target address.
static void SubBitField32(char* target_address, int lsb, int width, int32_t value) {
  int32_t mask = (1 << width) - 1;
  
  // Extract old value from word in lower bits.
  int32_t word = *(int32_t*)target_address;
  int32_t old_value = (word >> lsb) & mask;
  
  // Subtract value and mask with width.
  value = old_value - value;
  value &= mask;
  
  // Or in the new value at the bitfield position.
  mask <<= lsb;
  word &= ~mask;
  word |= value << lsb;
  *(int32_t*)target_address = word;
}

// A RISC-V J-Type instruction has a weird encoding for the immediate field.
// It is: imm[20|10:1|11|19:12]
// and is in bit position 12 (20 bits wide)
static void SetJTypeImm(char* target_address, int32_t immed) {
  int32_t encoded_value =  ((immed >> 20) & 1) << 31 |        // imm[20]
  ((immed >> 1) & 0x3ff) << 21 |   // imm[10:1]
  ((immed >> 11) & 1) << 20 |    // imm[11]
  ((immed >> 12) & 0xff) << 12;      // imm[19:12]
  int32_t word = *(int32_t*)target_address;
  word &= 0xfff;  // Preserve rd and opcode; replace imm[20|10:1|11|19:12].
  word |= encoded_value;
  *(int32_t*)target_address = word;
}

// Find the relocation associated with pc.  This is used to handle
// the R_RISCV_PCREL_LO12_X relocations that refer to an auipc instruction with
// a R_RISCV_PCREL_HI20 relocation.
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

static int32_t JTypeInstruction(int opcode, int rd, int immed) {
  return ((immed >> 20) & 1) << 31 |        // imm[20]
  ((immed >> 1) & 0x3ff) << 21 |            // imm[10:1]
  ((immed >> 11) & 1) << 20 |               // imm[11]
  ((immed >> 12) & 0xff) << 12 |            // imm[19:12]
  rd << 7 |                                 // rd
  opcode;                                   // opcode
}

static int32_t ITypeInstruction(int opcode, int rd, int rs1, int funct3,
                                int immed) {
  return immed << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | opcode;
}

static int32_t RTypeInstruction(int opcode, int rd, int rs1, int rs2,
                                int funct3, int funct7) {
  return funct7 << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | opcode;
}

static int32_t UTypeInstruction(int opcode, int rd, int immed) {
  return immed << 12 | rd << 7 | opcode;
}

// High 20 bits of a value.  Since the immed field encoded in the
// instructions that use the low 12 bits is signed, we need to make
// use of the max range by adding 0x800 to the value.  This moves the
// value to the middle of the 12-bit range and thus the low 12 bits
// can be negative.
static int32_t High20(int64_t val) {
  return ((int32_t)val + 0x800) >> 12;
}

static int32_t Low12(int64_t val, int32_t hi20) {
  return (int32_t)val - (hi20 << 12);
}

static void SplitValue(int64_t val, int32_t* hi20, int32_t* lo12) {
  *hi20 = High20(val);
  *lo12 = Low12(val, *hi20);
}

// Apply a RISC-V relocation.  S is the value of the symbol and A is the
// addend value (added to the symbol value).
static void ApplyRelocation(Linker* linker,
                            ObjectFile* file,
                            Relocation* reloc,
                            LinkerSymbol* symbol,
                            char* target_address,
                            uint64_t S, int64_t A) {
  // See https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
  // for details on the relocation calculations.
  uint64_t P = reloc->section->address + reloc->offset;
  int32_t hi20;
  int32_t lo12;
  int64_t tprel = (int64_t)S + A + RISCV_TLS_TCB_SIZE;
  
  // NOTE: a break in this switch will result in a linker error due to an unsupported
  // relocation.  To support a relocation, use return, not break.
  switch (reloc->type) {
    case R_RISCV_NONE:
      return;
    case R_RISCV_32:
      *((int32_t*)target_address) = (int32_t)(S + A);
      return;
    case R_RISCV_64:
      *((int64_t*)target_address) = S + A;
      return;
    case R_RISCV_RELATIVE:
      break;
    case R_RISCV_COPY:
      break;
    case R_RISCV_JUMP_SLOT:
      // This is only in .so files and only applied at runtime.
      break;
    case R_RISCV_BRANCH:
      break;
    case R_RISCV_JAL: {
      int32_t offset = (int32_t)(S - P - A);
      SetJTypeImm(target_address, offset);
      return;
    }
    case R_RISCV_CALL: {
      // This refers to 2 instructions:
      // auipc (U-Type)
      // jalr (I-Type)
      int32_t offset = (int32_t)(S - P - A);
      SplitValue(offset, &hi20, &lo12);
      SetBitField32(target_address, 12, 20, hi20);
      SetBitField32(target_address+4, 20, 12, lo12);
      return;
    }
    case R_RISCV_RELAX: {
      // This will refer to an auipc/jalr pair.  If the immediate
      // field of the auipc instruction is zero we can replace it
      // by a jal and convert the jalr to a nop.  We also check that
      // the rd of the auipc and the rs1 of the jalr are the same
      // register.
      int32_t auipc = *(int32_t*)target_address;
      if ((auipc & 0x7f) != RV_OPCODE(auipc)) {
        return;
      }
      int64_t immed = auipc >> 12;
      int auipc_rd = (auipc >> 7) & 0xff;
      if (immed == 0) {
        int32_t jalr = *(int32_t*)(target_address + 4);
        if ((jalr & 0x7f) != RV_OPCODE(jalr)) {
          return;
        }
        immed = jalr >> 20;
        int rs1 = (jalr >> 15) & 0x1f;
        int rd = (jalr >> 7) & 0x1f;
        if (rs1 == auipc_rd) {
          *(int32_t*)target_address = JTypeInstruction(RV_OPCODE(jal), rd, (int32_t)immed);
          *(int32_t*)(target_address+4) = RV_OPCODE(op_imm);  // addi x0,x0,0
        }
      }
      return;
    }
    case R_RISCV_PCREL_HI20: {
      int32_t offset = (int32_t)(S - P - A);
      SplitValue(offset, &hi20, &lo12);
      SetBitField32(target_address, 12, 20, hi20);
      return;
    }
      
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S: {
      // This relocation points to a label that contains the PCREL_HI20
      // relocation, that is used to get the symbol.  This is because
      // the instruction pair (auipc/ld) do not have to be adjacent.
      // We find the relocation associated with the symbol value of this
      // relocation and then use that to get the actual symbol being relocated.
      // The LO12 value is the low 12 bits for the PC offset from the label,
      // not the current PC value.
      Relocation* hi20_reloc = FindRelocation(linker, S);
      if (hi20_reloc == NULL) {
        LinkerError(file, "Unable to locate R_RISCV_PCREL_HI20 relocation");
        return;
      }
      
      // Get the symbol from the hi20 relocation.
      symbol = ObjectFileFindSymbol(file, hi20_reloc->symbol_name.value);
      if (symbol == NULL) {
        // Trying to apply relocation for undefined symbol
        LinkerError(file, "Undefined symbol %s", hi20_reloc->symbol_name.value);
        return;
      }
      
      // Difference in PC to auipc instruction.
      int32_t pcdiff = (int32_t)(reloc->offset - hi20_reloc->offset);
      S = symbol->address;
      
      if (hi20_reloc->type == R_RISCV_GOT_HI20) {
        // Refers to Global offset table relocation.
        uint64_t got_address = linker->dynamic_linker->
            got_group->address;
        // Each GOT entry is 8 bytes long.
        uint64_t addr = got_address + symbol->got_index * 8 + A;
        int32_t offset = (int32_t)(addr - P + pcdiff);
        SplitValue(offset, &hi20, &lo12);
      } else {
        int32_t offset = (int32_t)(S - P + pcdiff - A);
        SplitValue(offset, &hi20, &lo12);
      }
      
      if (reloc->type == R_RISCV_PCREL_LO12_S) {
        SetBitField32(target_address, 7, 5, lo12);
        SetBitField32(target_address, 25, 7, lo12 >> 5);
      } else {
        SetBitField32(target_address, 20, 12, lo12);
      }
      
      return;
    }
      
    case R_RISCV_CALL_PLT: {
      // LinkerSymbol contains a got_index that is the offset into the
      // PLT.  The instruction will be an call
      // instruction that contains the offset relative to the current
      // PC.
      uint64_t plt_address = linker->dynamic_linker->
          plt_group->address;
      
      // Each PLT entry is 16 bytes long.
      uint64_t addr = plt_address + symbol->plt_index * 16 + A;
      int32_t offset = (int32_t)(addr - P);
      SplitValue(offset, &hi20, &lo12);
      SetBitField32(target_address, 12, 20, hi20);
      SetBitField32(target_address+4, 20, 12, lo12);
      return;
    }
    case R_RISCV_GOT_HI20: {
      // LinkerSymbol contains a got_index that is the offset into the
      // global offset table.  The instruction will be an auipc
      // instruction that contains the offset relative to the current
      // PC.
      uint64_t got_address = linker->dynamic_linker->
          got_plt_group->address;
      // Each GOT entry is 8 bytes long.
      uint64_t addr = got_address + symbol->got_index * 8 + A;
      int32_t offset = (int32_t)(addr - P);
      hi20 = High20(offset);
      SetBitField32(target_address, 12, 20, hi20);
      return;
    }
    case R_RISCV_TPREL_HI20:
      hi20 = High20(tprel);
      SetBitField32(target_address, 12, 20, hi20);
      return;
    case R_RISCV_TPREL_LO12_I:
      hi20 = High20(tprel);
      lo12 = Low12(tprel, hi20);
      SetBitField32(target_address, 20, 12, lo12);
      return;
    case R_RISCV_TPREL_LO12_S:
      SplitValue(tprel, &hi20, &lo12);
      SetBitField32(target_address, 7, 5, lo12);
      SetBitField32(target_address, 25, 7, lo12 >> 5);
      return;
    case R_RISCV_TPREL_ADD:
      return;
    case R_RISCV_HI20:
      hi20 = High20(S + A);
      SetBitField32(target_address, 12, 20, hi20);
      return;
    case R_RISCV_LO12_I:
      hi20 = High20(S + A);
      lo12 = Low12(S + A, hi20);
      SetBitField32(target_address, 20, 12, lo12);
      return;
    case R_RISCV_LO12_S:
      SplitValue(S + A, &hi20, &lo12);
      SetBitField32(target_address, 7, 5, lo12);
      SetBitField32(target_address, 25, 7, lo12 >> 5);
      return;
    case R_RISCV_ADD8:
      AddBitField32(target_address, 0, 8, (int32_t)(S + A));
      return;
    case R_RISCV_ADD16:
      AddBitField32(target_address, 0, 16, (int32_t)(S + A));
      return;
    case R_RISCV_ADD32:
      *((int32_t*)target_address) += (int32_t)(S + A);
      return;
    case R_RISCV_ADD64:
      *((int64_t*)target_address) += S + A;
      return;
    case R_RISCV_SUB8:
      SubBitField32(target_address, 0, 8, (int32_t)(S + A));
      return;
    case R_RISCV_SUB16:
      SubBitField32(target_address, 0, 16, (int32_t)(S + A));
      return;
    case R_RISCV_SUB32:
      *((int32_t*)target_address) -= (int32_t)(S + A);
      return;
    case R_RISCV_SUB64:
      *((int64_t*)target_address) -= S + A;
      return;
    case R_RISCV_ALIGN:
      break;
    case R_RISCV_RVC_BRANCH:
      break;
    case R_RISCV_RVC_JUMP:
      break;
    case R_RISCV_RVC_LUI:
      break;
    default:
      break;
  }
  LinkerError(file, "Unsupported RISC-V relocation type %d", reloc->type);
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
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
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
  Relocation* reloc = NewLinkerSymbolRelocation(symbol,
                                          offset, reloc_type, 0);
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


// The RISC-V PLT trampoline is as follows:
// 1:  auipc   t3, %pcrel_hi(function@.got.plt)
// ld  t3, %pcrel_lo(1b)(t3)
// jalr    t1, t3
// nop
//
// It sets t3 to the address of the .got.plt entry for the
// function then loads it with the contents of that address
// (the GOT entry).  Then it jumps to that location, storing
// the address of the nop instruction in reg t1.  In lazy
// symbol resolution, the GOT entry will contain the address
// of the first PLT entry and reg t1 will contain an
// indication of which PLT entry needs to be resolved, from which
// the relocation can be determined and thus the symbol.
//
// NOTE: at this point we don't know the addresses of the .got.plt
// or the .plt sections.  These are known later and are passed
// to the FixupPLTEntry function.
static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  int32_t word;

  const int t1 = RV_INT_TEMP_START_1 + 1;
  const int t3 = RV_INT_TEMP_START_2 + 0;
  
  // auipc t3, auipc   t3, %pcrel_hi(symbol)
  word = UTypeInstruction(RV_OPCODE(auipc), t3, 0);
  BufferAppendWordLE(&contents->data.buffered, word);
  
  // ld t3, %pcrel_lo(1b)(t3)
  word = ITypeInstruction(RV_OPCODE(load), t3, t3, RV_F3(ld), 0);
  BufferAppendWordLE(&contents->data.buffered, word);

  // jalr t1, t3
  word = ITypeInstruction(RV_OPCODE(jalr), t1, t3,
                          RV_F3(jalr), 0);
  BufferAppendWordLE(&contents->data.buffered, word);
  
  // nop
  word = RV_OPCODE(op_imm);     // NOP instruction.
  BufferAppendWordLE(&contents->data.buffered, word);
}

// The first PLT entry occupies two slots and contains:
// This is from:
// https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
//
// 1:   auipc  t2, %pcrel_hi(.got.plt)
// sub    t1, t1, t3               # shifted .got.plt offset + hdr size + 12
// ld t3, %pcrel_lo(1b)(t2)    # _dl_runtime_resolve
// addi   t1, t1, -(hdr size + 12) # shifted .got.plt offset
// addi   t0, t2, %pcrel_lo(1b)    # &.got.plt
// srli   t1, t1, log2(16/PTRSIZE) # .got.plt offset
// ld t0, PTRSIZE(t0)          # link map
// jr     t3
//
// For reference, a regular PLT entry is coded as:
// 1:  auipc   t3, %pcrel_hi(function@.got.plt)
// ld  t3, %pcrel_lo(1b)(t3)
// jalr    t1, t3
// nop

// There's a lot of information here and it's obscure.  Let's try to
// decode it:
// shifted .got.plt offset is the byte difference between the PLT
//   entry and the start of the .plt section.  This is because
//   t3 contains the value of the .got.plt entry which is initially
//   set to the address of the start of the PLT (this code)
// hdr_size is 32, the size of the first PLT entry
// 12 bytes is the offset from the 'nop' instruction in the PLT entry
//    from the start of that entry (the difference between t1 and the
//    start of the entry).
// _dl_runtime_resolve is the address of the runtime resolver function
// PTRSIZE is 8
//
// So the code does this:
// 1. The first entry in the .got.plt section contains 2 64-bit words:
//    [0]: the address of the runtime resolver function
//    [1]: the "link_map" which is just data for the runtime resolver.
// 2. Load t2 with the high 20 bits of the .got.plt section address
// 3. Calculate the byte offset from the 'nop' instruction in the PLT
//    entry to the start of the .plt (the address of the resolver code
//    in the .plt).  Put this in t1.
// 4. Load t3 with the address of the runtime resolver.
// 5. Subtract 32+12 from t1 to get the actual byte offset from the PLT
//    entry to the start of the PLT.
// 6. Set t0 to the full address of the .got.plt
// 7. Divide the offset in t1 by 2 to get the offset into the .got.plt in
//    bytes.  This is the byte offset of the .got.plt entry that will
//    normally contain the address of the function to call.
// 8. Load t0 with the "link_map" (resolver data)
// 9. Jump to the resolver.
//
// So on entry to the resolver:
// t0 contains the resolver data
// t1 contains the byte offset from the start of the .got.plt for the
//    function to call, which can be used to determine the relocation
//    and thus the symbol.
static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer,
                                  uint64_t got_address,
                                  uint64_t plt_address) {
  const int t0 = RV_INT_TEMP_START_1 + 0;
  const int t1 = RV_INT_TEMP_START_1 + 1;
  const int t2 = RV_INT_TEMP_START_1 + 2;
  const int t3 = RV_INT_TEMP_START_2 + 0;

  uint32_t* p = (uint32_t*)plt_buffer->value;

  int32_t addr_diff = (int32_t)(got_address - plt_address);
  int32_t hi20, lo12;
  SplitValue(addr_diff, &hi20, &lo12);
  
  // 1:   auipc  t2, %pcrel_hi(.got.plt)
  p[0] = UTypeInstruction(RV_OPCODE(auipc), t2, hi20);
  
  // sub    t1, t1, t3               # shifted .got.plt offset + hdr size + 12
  p[1] = RTypeInstruction(RV_OPCODE(op), t1, t1, t3, RV_F3(sub), RV_F7(sub));

  // ld t3, %pcrel_lo(1b)(t2)    # _dl_runtime_resolve
  p[2] = ITypeInstruction(RV_OPCODE(load), t3, t2, RV_F3(ld), lo12);
  
  // addi   t1, t1, -(hdr size + 12) # shifted .got.plt offset
  p[3] = ITypeInstruction(RV_OPCODE(op_imm), t1, t1, RV_F3(addi), -(32+12));
  
  // addi   t0, t2, %pcrel_lo(1b)    # &.got.plt
  p[4] = ITypeInstruction(RV_OPCODE(op_imm), t0, t2, RV_F3(addi), lo12);
  
  // srli   t1, t1, log2(16/PTRSIZE) # .got.plt offset
  p[5] = RTypeInstruction(RV_OPCODE(op_imm), t1, t1, 1, RV_F3(srli), RV_F7(srli));
  
  // ld t0, PTRSIZE(t0)          # link map
  p[6] = ITypeInstruction(RV_OPCODE(load), t0, t0, RV_F3(ld), 8);
  
  // jr     t3
  p[7] = ITypeInstruction(RV_OPCODE(jalr), 0, t3,
                          RV_F3(jalr), 0);
}

static void FixupPLTEntry(ProcedureLinkageTable* plt,
                          GlobalOffsetTable* got,
                          LinkerSymbol* symbol,
                          Buffer* plt_buffer,
                          uint64_t got_address,
                          uint64_t plt_address) {
  uint64_t offset = symbol->plt_index * (int)plt->entry_size;
  uint64_t entry_address = got_address +
      symbol->got_index * got->entry_size;
  
  // The first two instructions in the plt trampoline are:
  // 1:  auipc   t3, %pcrel_hi(function@.got.plt)
  // ld  t3, %pcrel_lo(1b)(t3)

  uint64_t trampoline_address = plt_address + offset;
  uint32_t* p = (uint32_t*)(plt_buffer->value + offset);
  
  // Offset in bytes from trampoline start to got entry address.
  int64_t pcrel = entry_address - trampoline_address;
  
  int32_t hi20, lo12;
  SplitValue(pcrel, &hi20, &lo12);
  
  // auipc instruction has the upper 20 bits of the offset in its
  // upper 20 bits.
  p[0] |= hi20 << 12;
  
  // ld instruction, upper 12 bits are lower 12 bits of offset.
  p[1] |= lo12 << 20;
}

static void CheckOptions(Linker* linker) {
}

LinkerArchitecture* NewRISCVLinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->machine_type = ELF_MACHINE_TYPE_RISC_V;
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
