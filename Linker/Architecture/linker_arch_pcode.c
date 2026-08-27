//
//  linker_arch_pcode.c
//  c_compiler
//
//  Created by David Allison on 3/7/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdlib.h>
#include "linker_arch_pcode.h"

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
    case R_PCODE_GOT_ENTRY:
    case R_PCODE_GOT_TLS_IE:
      symbol->got_index = append_data_to_got(dynamic, symbol);
      break;
 
    case R_PCODE_GOT_TLS_GD:
      // This needs two GOT entries.
      symbol->got_index = append_data_to_got(dynamic, symbol);
      append_data_to_got(dynamic, symbol);
      symbol->got_index--;    // Back to first entry.
      break;
      
    case R_PCODE_CALL_PLT:
      // Add GOT entry for function address.
      symbol->got_index = append_func_to_got(dynamic, symbol);
      
      // Add PLT entry for call to GOT.
      symbol->plt_index = append_to_plt(dynamic, symbol);
      break;
      
    case R_PCODE_DATA64: {
      // A data word holding an address.
      Relocation* rel_reloc = NewDataAddressRelocation(
          symbol, reloc, R_PCODE_RELATIVE, R_PCODE_GOT_DATA);
      VectorAppend(&dynamic->data_relocations, rel_reloc);
      break;
    }
  }
}

static void ApplyRelocation(Linker* linker,
                          ObjectFile* file,
                          Relocation* reloc,
                          LinkerSymbol* symbol,
                          char* target_address,
                          uint64_t S, int64_t A) {
  // PC in P-Code is address of next instruction.  All the PC relative
  // instructions are 96 bits long.
  uint64_t P = reloc->section->address + reloc->offset + 12;
  switch (reloc->type) {
    case R_PCODE_TLS_TP_OFF:    // TLS Thread pointer offset.
    case R_PCODE_ABS: {
      // Instruction is 96 bits long.  The relocation is applied
      // to the second and third word, in little endian format.
      // The instruction can be either a movxc or an adr.  In the latter
      // case the value stored is pc relative.
      int opcode = target_address[3] & 0x3f;
      if (opcode == PCODE_OP(movxc)) {    // MOVXC
        *((uint64_t*)(target_address + 4)) = S + A;
      } else if (opcode == PCODE_OP(adr)) {   // ADR
        *((uint64_t*)(target_address + 4)) = S + A - P;
      } else {
        LinkerError(file, "Unsupported P-CODE ABS relocation opcode %d", opcode);
      }
      break;
    }
      
    case R_PCODE_JMP:
    case R_PCODE_CALL: {
      // PC-relative instructions.
      *((uint64_t*)(target_address + 4)) = S + A - P;
      break;
    }
      
    case R_PCODE_DATA64:
      *((uint64_t*)(target_address)) = S + A;
      break;
      
    case R_PCODE_DATA32:
      *((uint32_t*)(target_address)) = (uint32_t)(S + A);
      break;
      
    case R_PCODE_CALL_PLT: {
      // Instruction is 96 bits long.  The relocation is applied
      // to the second and third word, in little endian format.
      // LinkerSymbol contains a got_index that is the offset into the
      // PLT.  The instruction will be an CALL
      // instruction that contains the offset relative to the current
      // PC.
      //
      // A fully static link builds no PLT because there is no runtime resolver
      // for a trampoline to reach; the callee's address is final, so the call
      // goes straight to it.
      uint64_t addr;
      if (linker->dynamic_linker == NULL || symbol == NULL ||
          symbol->plt_index < 0 ||
          linker->dynamic_linker->plt_group == NULL) {
        addr = S + (uint64_t)A;
      } else {
        // Each PLT entry is 36 bytes long.
        addr = linker->dynamic_linker->plt_group->address +
               (uint64_t)symbol->plt_index * 36 + (uint64_t)A;
      }
      *((uint64_t*)(target_address + 4)) = addr - P;
      break;
    }
    case R_PCODE_GOT_TLS_IE:   // TLS IE GOT entry.
    case R_PCODE_GOT_TLS_GD:    // TLS GD GOT entry.
    case R_PCODE_GOT_ENTRY: {
      // Instruction is 96 bits long.  The relocation is applied
      // to the second and third word, in little endian format.
      // LinkerSymbol contains a got_index that is the offset into the
      // global offset table.  The instruction will be an ADR
      // instruction that contains the offset relative to the current
      // PC.
      uint64_t got_address = linker->dynamic_linker->
      got_group->address;
      // Each GOT entry is 8 bytes long.
      uint64_t addr = got_address + symbol->got_index * 8 + A;
      *((uint64_t*)(target_address + 4)) = addr - P;
      break;
    }
    case R_PCODE_GOT_DATA:
      break;
    case R_PCODE_GOT_FUNC:
      break;
    case R_PCODE_PCREL:
      *((uint64_t*)(target_address + 4)) = S + A - P;
      break;
    default:
      LinkerError(file, "Unsupported P-CODE relocation type %d", reloc->type);
      break;
  }
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  // 2 reserved entries at the beginning of the PLTGOT:
  // 0: pointer to data structure for dynamic loader
  // 1: address of runtime resolver.
  dynamic->global_offset_table.num_resolver_data_entries = 2;
  dynamic->global_offset_table.entry_size = 8;
  
  // First entry of PLT contains jump to the runtime resolver.
  dynamic->procedure_linkage_table.num_reserved_entries = 1;
  dynamic->procedure_linkage_table.entry_size = 36;
}

#if 0
static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
                        int32_t relocation_type) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  int32_t reloc_type = is_function ? R_PCODE_GOT_FUNC : R_PCODE_GOT_DATA;
  int64_t offset = contents->data.buffered.length;
  
  BufferAppendLongLE(&contents->data.buffered, 0);
  
  // Add relocation.
  Relocation* reloc = NewLinkerSymbolRelocation(symbol,
                                          offset, relocation_type, 0);
  VectorAppend(is_function ?
               &dynamic->plt_relocations :
               &dynamic->got_relocations,
               reloc);
}
#endif

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
                        GOTRelocation relocation_type) {
  int64_t offset = contents->data.buffered.length;
  int32_t reloc_type;
  switch (relocation_type) {
    case kGOTRelocationFunction:
      reloc_type = R_PCODE_GOT_FUNC;
      break;
    case kGOTRelocationVariable:
      reloc_type = R_PCODE_GOT_DATA;
      break;
    case kGOTRelocationTLSOffset:
      reloc_type = R_PCODE_GOT_TLS_OFFSET;
      break;
    case kGOTRelocationTLSModuleId:
      reloc_type = R_PCODE_GOT_TLS_MODID;
      break;
}
  BufferAppendLongLE(&contents->data.buffered, 0);
  
  // Add relocation.
  Relocation* reloc = NewLinkerSymbolRelocation(symbol,
                                          offset, reloc_type, 0);
  VectorAppend(relocs, reloc);
}

static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  // The address is set to the address of the plt +
  //    the plt_index * plt entry size + 12.  That is, it points to the
  // second instruction of the PLT entry.
  uint64_t addr = plt_address + symbol->plt_index * plt_entry_size + 12;
  uint64_t* p = (uint64_t*)got_plt_buffer->value + symbol->got_index;
  *p = addr;
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  DynamicLinker* dynamic = linker->dynamic_linker;
  ProcedureLinkageTable* plt = &dynamic->procedure_linkage_table;
  int32_t word;
  // For P-Code a PLT entry is as follows:
  // cjmp got_entry_address
  // movxc t1, #got_index
  // jmp entry_0
  //
  // Initially, the GOT entry for a function contains the
  // address of the movc instruction.  The first call to
  // this PLT trampoline will jump to the movc and then on
  // to _dl_runtime_resolve that will use the value in the
  // tmp register to determine the offset into the GOT
  // and the value on the stack (return address from the
  // call instruction) to determine the symbol.  It will
  // look up the symbol and overwrite the GOT entry with
  // the symbol value, then jump to it.
  //
  
  // cjmp got_entry_address
  word = 0xc0000000 | PCODE_OP(cjmp) << 24;
  BufferAppendWordLE(&contents->data.buffered, word);
  BufferAppendWordLE(&contents->data.buffered, 0);
  BufferAppendWordLE(&contents->data.buffered, 0);
  
  // movc t1, #got_index
  word = 0xc0000000 | PCODE_OP(movxc) << 24 | PCODE_TMP1_REG << 16;
  BufferAppendWordLE(&contents->data.buffered, word);
  BufferAppendWordLE(&contents->data.buffered, symbol->got_index -
                     dynamic->global_offset_table.num_resolver_data_entries);
  BufferAppendWordLE(&contents->data.buffered, 0);
  
  // jmp entry_0
  // This a negative jump back to the start of the PLT.
  word = 0xc0000000 | PCODE_OP(jmp) << 24;
  BufferAppendWordLE(&contents->data.buffered, word);
  // Offset from this jmp instruction to start of plt.  The jmp instruciton
  // is the third in the tramploline and the offset is from the PC when
  // this instruction is executed, which is the next instruction.  So the
  // calculation is:
  // the negative of:
  //  the start of this trampoline entry (plt_index * entry_size) +
  //  the size of an entry
  int64_t entry_0_offset = -(symbol->plt_index * plt->entry_size +
                             plt->entry_size);
  BufferAppendWordLE(&contents->data.buffered, entry_0_offset & 0xffffffffLL);
  BufferAppendWordLE(&contents->data.buffered, entry_0_offset >> 32LL);
}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer,
                                  uint64_t got_address,
                                  uint64_t plt_address) {
  // The GOT entry for _dl_runtime_resolve is at index 1.  This code
  // loads the value at index 0 (sometimes called the link_map) into
  // t2 then jumps to the GOT entry.  We can't use reg t1
  // since this contains the index into the GOT from the trampoline.
  //
  // The machine code is:
  // adr t2, got
  // cjmp got + 8
  //
  uint32_t* p = (uint32_t*)plt_buffer->value;
  
  uint32_t word = 0xc0000000 | PCODE_OP(adr) << 24 | PCODE_TMP2_REG << 16;
  p[0] = word;
  
  // Relative to end of adr instruction.
  int64_t pcrel = got_address - (plt_address + 12);
  
  p[1] = pcrel & 0xffffffffLL;
  p[2] = pcrel >> 32;
  
  word = 0xc0000000 | PCODE_OP(cjmp) << 24;
  
  p[3] = word;
  
  // Relative to instruction after cjmp.
  pcrel = got_address + 8 - (plt_address + 12 + 12);
  
  p[4] = pcrel & 0xffffffffLL;
  p[5] = pcrel >> 32;
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
  
  // The first instruction is a cjmp instruction
  // that contains a PC-relative offset to the GOT entry.
  uint64_t trampoline_address = plt_address + offset;
  uint32_t* p = (uint32_t*)(plt_buffer->value + offset + 4);
  int64_t pcrel = entry_address - (trampoline_address + 12);
  p[0] = pcrel & 0xffffffffLL;
  p[1] = pcrel >> 32;
 
}

static void CheckOptions(Linker* linker) {
}

LinkerArchitecture* NewPCodeLinkerArchitecture() {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->machine_type = ELF_MACHINE_TYPE_PCODE;
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
