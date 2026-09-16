//
//  linker_arch_x86.c
//  c_compiler
//

#include "linker_arch_x86.h"

#include <stdlib.h>
#include <string.h>

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    address = LINKER_DSO_CODE_SEGMENT_START_ADDRESS +
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

static void HandlePICRelocation(DynamicLinker* dynamic, LinkerSymbol* symbol,
                                Relocation* reloc,
                                int (*append_data_to_got)(DynamicLinker*,
                                                          LinkerSymbol*),
                                int (*append_func_to_got)(DynamicLinker*,
                                                          LinkerSymbol*),
                                int (*append_to_plt)(DynamicLinker*,
                                                     LinkerSymbol*)) {
  switch (reloc->type) {
    case R_386_GOT32:
    case R_386_GOT32X:
      if (symbol != NULL) {
        symbol->got_index = append_data_to_got(dynamic, symbol);
      }
      break;
    case R_386_PLT32:
      if (symbol != NULL &&
          (!symbol->defined || symbol->section == NULL)) {
        symbol->got_index = append_func_to_got(dynamic, symbol);
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;
    case R_386_32: {
      Relocation* dynamic_reloc = NewDataAddressRelocation(
          symbol, reloc, R_386_RELATIVE, R_386_32);
      VectorAppend(&dynamic->data_relocations, dynamic_reloc);
      break;
    }
    default:
      break;
  }
}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t P = reloc->section->address + reloc->offset;
  int32_t pc32;
  (void)file;

  switch (reloc->type) {
    case R_386_NONE:
      return;

    case R_386_32:
      *((int32_t*)target_address) =
          (int32_t)(S + A +
                    (reloc->addend_in_place
                         ? *(const int32_t*)target_address
                         : 0));
      return;

    case R_386_PC32: {
      int32_t implicit_addend =
          reloc->addend_in_place ? *(const int32_t*)target_address : 0;
      pc32 = (int32_t)(S + A + implicit_addend - P);
      *((int32_t*)target_address) = pc32;
      return;
    }

    case R_386_PLT32: {
      int32_t implicit_addend =
          reloc->addend_in_place ? *(const int32_t*)target_address : 0;
      uint64_t B = S;
      if (symbol != NULL && symbol->plt_index >= 0 &&
          linker->dynamic_linker != NULL &&
          linker->dynamic_linker->plt_group != NULL) {
        B = linker->dynamic_linker->plt_group->address +
            (uint64_t)symbol->plt_index * 16;
      }
      pc32 = (int32_t)(B + A + implicit_addend - P);
      *((int32_t*)target_address) = pc32;
      return;
    }

    case R_386_GOT32:
    case R_386_GOT32X: {
      int32_t implicit_addend =
          reloc->addend_in_place ? *(const int32_t*)target_address : 0;
      uint64_t got_base = 0;
      uint64_t addr = S;
      if (symbol != NULL && symbol->got_index >= 0 &&
          linker->dynamic_linker != NULL &&
          linker->dynamic_linker->got_group != NULL) {
        got_base = linker->dynamic_linker->got_plt_group->address;
        addr = linker->dynamic_linker->got_group->address +
               (uint64_t)symbol->got_index * 4;
      }
      *((int32_t*)target_address) =
          (int32_t)(addr + A + implicit_addend - got_base);
      return;
    }

    case R_386_GOTPC: {
      int32_t implicit_addend =
          reloc->addend_in_place ? *(const int32_t*)target_address : 0;
      uint64_t got =
          linker->dynamic_linker != NULL &&
                  linker->dynamic_linker->got_plt_group != NULL
              ? linker->dynamic_linker->got_plt_group->address
              : 0;
      *((int32_t*)target_address) =
          (int32_t)(got + A + implicit_addend - P);
      return;
    }

    case R_386_GLOB_DAT:
      *((int32_t*)target_address) = (int32_t)(S + A);
      return;

    case R_386_JMP_SLOT:
    case R_386_RELATIVE:
    case R_386_COPY:
      return;

    default:
      LinkerError(file, "unsupported i386 relocation type %d", reloc->type);
      return;
  }
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  // SysV i386 reserves GOT[0] for _DYNAMIC, GOT[1] for the link map and
  // GOT[2] for the runtime resolver.  PLT entries are 16 bytes.
  dynamic->global_offset_table.num_resolver_data_entries = 3;
  dynamic->global_offset_table.entry_size = 4;
  dynamic->procedure_linkage_table.num_reserved_entries = 1;
  dynamic->procedure_linkage_table.entry_size = 16;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents, Vector* relocs,
                        GOTRelocation relocation_type) {
  (void)linker;
  int32_t reloc_type;
  bool by_symbol;
  switch (relocation_type) {
    case kGOTRelocationFunction:
      by_symbol = true;
      reloc_type = R_386_JMP_SLOT;
      break;
    case kGOTRelocationVariable:
      by_symbol = symbol->from_dynamic_library;
      reloc_type = by_symbol ? R_386_GLOB_DAT : R_386_RELATIVE;
      break;
    case kGOTRelocationTLSOffset:
    case kGOTRelocationTLSModuleId:
      LinkerError(NULL, "i386 TLS dynamic relocations are not implemented");
      linker->num_errors++;
      return;
  }
  int64_t offset = (int64_t)contents->data.buffered.length;
  BufferAppendWordLE(&contents->data.buffered, 0);
  Relocation* reloc =
      NewLinkerSymbolRelocation(symbol, offset, reloc_type, 0);
  reloc->resolve_by_symbol = by_symbol;
  VectorAppend(relocs, reloc);
}

static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  (void)plt_entry_size;
  uint32_t* p = (uint32_t*)got_plt_buffer->value + symbol->got_index;
  *p = (uint32_t)(plt_address + (uint64_t)symbol->plt_index * 16 + 6);
}

static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  (void)linker;
  (void)symbol;
  // Position-independent eager-binding trampoline:
  //   call 1f; 1: popl %eax; jmp *GOT_SLOT-1b(%eax); nopl 0(%eax)
  BufferAppendByte(&contents->data.buffered, 0xe8);
  BufferAppendWordLE(&contents->data.buffered, 0);
  BufferAppendByte(&contents->data.buffered, 0x58);
  BufferAppendByte(&contents->data.buffered, 0xff);
  BufferAppendByte(&contents->data.buffered, 0xa0);
  BufferAppendWordLE(&contents->data.buffered, 0);
  BufferAppendByte(&contents->data.buffered, 0x0f);
  BufferAppendByte(&contents->data.buffered, 0x1f);
  BufferAppendByte(&contents->data.buffered, 0x40);
  BufferAppendByte(&contents->data.buffered, 0x00);
}

static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {
  (void)plt;
  (void)plt_address;
  uint8_t* p = (uint8_t*)plt_buffer->value;
  // pushl GOT[1]; jmp *GOT[2]; four-byte padding
  p[0] = 0xff;
  p[1] = 0x35;
  *(uint32_t*)(p + 2) = (uint32_t)(got_address + 4);
  p[6] = 0xff;
  p[7] = 0x25;
  *(uint32_t*)(p + 8) = (uint32_t)(got_address + 8);
  p[12] = 0x0f;
  p[13] = 0x1f;
  p[14] = 0x40;
  p[15] = 0x00;
}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {
  uint64_t offset =
      (uint64_t)symbol->plt_index * (uint64_t)plt->entry_size;
  uint64_t trampoline_address = plt_address + offset;
  uint8_t* p = (uint8_t*)(plt_buffer->value + offset);
  uint64_t slot_address =
      got_address +
      (uint64_t)symbol->got_index * (uint64_t)got->entry_size;
  *(int32_t*)(p + 8) =
      (int32_t)(slot_address - (trampoline_address + 5));
}

static void CheckOptions(Linker* linker) {
  // i386 PLT entries are fully position-independent by using a local
  // call/pop pair.  They intentionally omit the legacy EBX-based lazy
  // resolver sequence, so all JUMP_SLOT entries must be resolved at load.
  linker->bind_now = true;
}

LinkerArchitecture* NewX86LinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->machine_type = ELF_MACHINE_TYPE_X86;
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
