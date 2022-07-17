//
//  linker_arch_6502.c
//  p_code_linker
//
//  Created by David Allison on 6/25/19.
//  Copyright © 2019 David Allison. All rights reserved.
//
#include <stdlib.h>
#include <inttypes.h>
#include "linker_arch_6502.h"

static int64_t CodeStartAddress(Linker* linker) {
  return linker->origin == 0 ?W65C02_CODE_START : linker->origin;
}

static int64_t DataStartAddress(Linker* linker, int64_t code_start, int64_t code_size) {
  return code_start + code_size;
}

static void HandlePICRelocation(DynamicLinker* dynamic, LinkerSymbol* symbol,
                                Relocation* reloc,
                                int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
                                int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
                                int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
}

static void ApplyRelocation(Linker* linker,
                            ObjectFile* file,
                            Relocation* reloc,
                            LinkerSymbol* symbol,
                            char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t value = S + A;
  if ((value & ~0xffff) != 0) {
    LinkerError(file, "Invalid relocation value 0x%" PRIx64 " doesn't fit in 16 bits", value);
  }
  switch (reloc->type) {
    case R_W65C02_JSR:    // JSR - second and third bytes in little endian.
    case R_W65C02_JMP:    // JMP
      *(uint16_t*)(target_address+1) = value;   // Apply to offset.
      break;
      
    case R_W65C02_DATA16:
      *((uint16_t*)(target_address)) = value;
      break;

    case R_W65C02_DATA64:
      *((uint64_t*)(target_address)) = value;
      break;
      
    case R_W65C02_DATA32:
      *((uint32_t*)(target_address)) = (uint32_t)value;
      break;
      
    case R_W65C02_BYTE0:
      *(target_address) = value & 0xff;
      break;
  
    case R_W65C02_BYTE1:
      *(target_address) = (value >> 8) & 0xff;
      break;

    case R_W65C02_BYTE2:
      *(target_address) = (value >> 16) & 0xff;
      break;
      
    case R_W65C02_BYTE3:
      *(target_address) = (value >> 24) & 0xff;
      break;
      
    case R_W65C02_BYTE4:
      *(target_address) = (value >> 32) & 0xff;
      break;
      
    case R_W65C02_BYTE5:
      *(target_address) = (value >> 40) & 0xff;
      break;
      
    case R_W65C02_BYTE6:
      *(target_address) = (value >> 48) & 0xff;
      break;
      
    case R_W65C02_BYTE7:
      *(target_address) = (value >> 56) & 0xff;
      break;
      
   default:
      LinkerError(file, "Unsupported 6502 relocation type %d", reloc->type);
      break;
  }
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  dynamic->global_offset_table.num_resolver_data_entries = 0;
  dynamic->global_offset_table.entry_size = 0;
  
  dynamic->procedure_linkage_table.num_reserved_entries = 0;
  dynamic->procedure_linkage_table.entry_size = 0;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        Vector* relocs,
                        GOTRelocation relocation_type) {

}

static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {

}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer,
                                  uint64_t got_address,
                                  uint64_t plt_address) {
 
}

static void FixupPLTEntry(ProcedureLinkageTable* plt,
                          GlobalOffsetTable* got,
                          LinkerSymbol* symbol,
                          Buffer* plt_buffer,
                          uint64_t got_address,
                          uint64_t plt_address) {
 
  
}

static void CheckOptions(Linker* linker) {
  if (linker->building_dso) {
    fprintf(stderr, "Shared libraries are not supported on 6502\n");
    exit(1);
  }
  if (!linker->fully_static) {
    fprintf(stderr, "Only static executables can be build on the 6502 target (pass -static)\n");
    exit(1);
  }
}

LinkerArchitecture* New6502LinkerArchitecture() {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->machine_type = ELF_MACHINE_TYPEW65C02;
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
