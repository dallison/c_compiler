//
//  linker_arch_x86_64.c
//  c_compiler
//

#include "linker_arch_x86_64.h"

#include <stdlib.h>
#include <string.h>

#ifndef R_X86_64_GOTPCRELX
#define R_X86_64_GOTPCRELX 41
#endif
#ifndef R_X86_64_REX_GOTPCRELX
#define R_X86_64_REX_GOTPCRELX 42
#endif

static int64_t CodeStartAddress(Linker* linker) {
  int64_t address;
  if (linker->building_dso) {
    address = LINKER_DSO_CODE_SEGMENT_START_ADDRESS +
              LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
  } else {
    address = LINKER_CODE_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker) +
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
    address += LinkerSectionHeaderOffset(linker) + 1 * linker->ops->program_header_size;
  } else if (linker->fully_static) {
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker);
  } else {
    address = LINKER_DATA_SEGMENT_START_ADDRESS + LinkerSectionHeaderOffset(linker) +
              2 * linker->ops->program_header_size;
  }
  address += (linker->section_groups.length + LINKER_NUM_EXTRA_SECTIONS + 1) *
                 linker->ops->section_header_size +
             code_size;
  return address;
}

static void HandlePICRelocation(
    DynamicLinker* dynamic, LinkerSymbol* symbol, Relocation* reloc,
    int (*append_data_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_func_to_got)(DynamicLinker*, LinkerSymbol*),
    int (*append_to_plt)(DynamicLinker*, LinkerSymbol*)) {
  switch (reloc->type) {
    case R_X86_64_GOTPCREL:
    case R_X86_64_GOTPCRELX:
    case R_X86_64_REX_GOTPCRELX:
    case R_X86_64_GOT32:
      if (symbol != NULL) {
        symbol->got_index = append_data_to_got(dynamic, symbol);
      }
      break;

    case R_X86_64_PLT32:
      if (symbol != NULL) {
        symbol->got_index = append_func_to_got(dynamic, symbol);
        symbol->plt_index = append_to_plt(dynamic, symbol);
      }
      break;

    case R_X86_64_64: {
      Relocation* rel_reloc = NewDataAddressRelocation(
          symbol, reloc, R_X86_64_RELATIVE, R_X86_64_GLOB_DAT);
      VectorAppend(&dynamic->data_relocations, rel_reloc);
      break;
    }
  }
}

static void ApplyRelocation(Linker* linker, ObjectFile* file, Relocation* reloc,
                            LinkerSymbol* symbol, char* target_address,
                            uint64_t S, int64_t A) {
  uint64_t P = reloc->section->address + reloc->offset;
  int32_t pc32;

  // NOTE: a break in this switch will result in a linker error due to an
  // unsupported relocation.  To support a relocation, use return, not break.
  switch (reloc->type) {
    case R_X86_64_NONE:
      return;

    case R_X86_64_64:
      *((int64_t*)target_address) = (int64_t)(S + A);
      return;

    case R_X86_64_32:
      *((int32_t*)target_address) = (int32_t)(S + A);
      return;

    case R_X86_64_32S:
      *((int32_t*)target_address) = (int32_t)(S + A);
      return;

    case R_X86_64_16:
      *((int16_t*)target_address) = (int16_t)(S + A);
      return;

    case R_X86_64_PC64:
      *((int64_t*)target_address) = (int64_t)(S + A - P);
      return;

    case R_X86_64_PC32:
      pc32 = (int32_t)(S + A - P);
      *((int32_t*)target_address) = pc32;
      return;

    case R_X86_64_PC16:
      *((int16_t*)target_address) = (int16_t)(S + A - P);
      return;

    case R_X86_64_PC8:
      *((int8_t*)target_address) = (int8_t)(S + A - P);
      return;

    case R_X86_64_PLT32: {
      uint64_t B = S;
      if (symbol != NULL && symbol->plt_index >= 0 &&
          linker->dynamic_linker != NULL &&
          linker->dynamic_linker->plt_group != NULL) {
        B = linker->dynamic_linker->plt_group->address +
            (uint64_t)symbol->plt_index * 16;
      }
      pc32 = (int32_t)(B + A - P);
      *((int32_t*)target_address) = pc32;
      return;
    }

    case R_X86_64_GOTPCREL:
    case R_X86_64_GOTPCRELX:
    case R_X86_64_REX_GOTPCRELX:
    case R_X86_64_GOT32: {
      uint64_t addr = S + A;
      if (symbol != NULL && symbol->got_index >= 0 &&
          linker->dynamic_linker != NULL) {
        SectionGroup* got_group =
            symbol->plt_index >= 0 ? linker->dynamic_linker->got_plt_group
                                   : linker->dynamic_linker->got_group;
        if (got_group != NULL) {
          addr = got_group->address + (uint64_t)symbol->got_index * 8 +
                 (uint64_t)A;
        }
      }
      pc32 = (int32_t)(addr - P);
      *((int32_t*)target_address) = pc32;
      return;
    }

    case R_X86_64_COPY:
      break;

    case R_X86_64_GLOB_DAT:
      *((int64_t*)target_address) = (int64_t)(S + A);
      return;

    case R_X86_64_JUMP_SLOT:
      return;

    case R_X86_64_RELATIVE:
      return;

    case R_X86_64_TPOFF64:
      *((int64_t*)target_address) = (int64_t)(S + A + X86_64_TLS_TP_SLOT_SIZE);
      return;

    case R_X86_64_TPOFF32:
      *((int32_t*)target_address) = (int32_t)(S + A + X86_64_TLS_TP_SLOT_SIZE);
      return;

    default:
      break;
  }
  LinkerError(file, "Unsupported x86_64 relocation type %d", reloc->type);
}

static void InitDynamicLinker(DynamicLinker* dynamic) {
  // The x86-64 PLT reserves GOT[1] for the link map and GOT[2] for the
  // resolver address. Function slots therefore start at GOT[3].
  dynamic->global_offset_table.num_resolver_data_entries = 3;
  dynamic->global_offset_table.entry_size = 8;
  dynamic->procedure_linkage_table.num_reserved_entries = 1;
  dynamic->procedure_linkage_table.entry_size = 16;
}

static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents, Vector* relocs,
                        GOTRelocation relocation_type) {
  (void)linker;
  int32_t reloc_type;
  // A GOT slot only has to name its symbol when the loader is the one that
  // knows the address; see NewDataAddressRelocation for the same choice on an
  // ordinary data word.  A slot naming a symbol also needs that symbol in
  // .dynsym, which a definition private to this image has no reason to be in.
  bool by_symbol = true;
  switch (relocation_type) {
    case kGOTRelocationFunction:
      reloc_type = R_X86_64_JUMP_SLOT;
      break;
    case kGOTRelocationVariable:
      // A slot for one of the GOT-based TLS models holds an offset or a
      // module id rather than an address, and some targets allocate those out
      // of this same list, so they keep the form resolved by name.
      by_symbol = symbol->from_dynamic_library || LinkerSymbolIsTLS(symbol);
      reloc_type = by_symbol ? R_X86_64_64 : R_X86_64_RELATIVE;
      break;
    case kGOTRelocationTLSOffset:
    case kGOTRelocationTLSModuleId:
      reloc_type = R_X86_64_64;
      break;
  }
  int64_t offset = (int64_t)contents->data.buffered.length;
  BufferAppendLongLE(&contents->data.buffered, 0);
  Relocation* reloc = NewLinkerSymbolRelocation(symbol, offset, reloc_type, 0);
  reloc->resolve_by_symbol = by_symbol;
  VectorAppend(relocs, reloc);
}

static void FixupGOTEntry(LinkerSymbol* symbol, Buffer* got_plt_buffer,
                          uint64_t plt_address, int plt_entry_size) {
  (void)plt_entry_size;
  uint64_t* p = (uint64_t*)got_plt_buffer->value + symbol->got_index;
  *p = plt_address + (uint64_t)symbol->plt_index * 16 + 6;
}

// jmp *disp(%rip); pushq $0; jmp .
static void AddPLTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents) {
  (void)linker;
  (void)symbol;
  BufferAppendByte(&contents->data.buffered, 0xff);
  BufferAppendByte(&contents->data.buffered, 0x25);
  BufferAppendWordLE(&contents->data.buffered, 0);
  BufferAppendByte(&contents->data.buffered, 0x68);
  BufferAppendWordLE(&contents->data.buffered, 0);
  BufferAppendByte(&contents->data.buffered, 0xe9);
  BufferAppendWordLE(&contents->data.buffered, 0);
}

// pushq GOT+8(%rip); jmp *GOT+16(%rip); nopl 0x0(%rax)
static void SetupResolverPLTEntry(ProcedureLinkageTable* plt,
                                  Buffer* plt_buffer, uint64_t got_address,
                                  uint64_t plt_address) {
  (void)plt;
  uint8_t* p = (uint8_t*)plt_buffer->value;
  int32_t push_disp = (int32_t)((got_address + 8) - (plt_address + 6));
  int32_t jmp_disp = (int32_t)((got_address + 16) - (plt_address + 12));

  p[0] = 0xff;
  p[1] = 0x35;
  memcpy(p + 2, &push_disp, 4);
  p[6] = 0xff;
  p[7] = 0x25;
  memcpy(p + 8, &jmp_disp, 4);
  p[12] = 0x0f;
  p[13] = 0x1f;
  p[14] = 0x40;
  p[15] = 0x00;
}

static void FixupPLTEntry(ProcedureLinkageTable* plt, GlobalOffsetTable* got,
                          LinkerSymbol* symbol, Buffer* plt_buffer,
                          uint64_t got_address, uint64_t plt_address) {
  uint64_t offset = (uint64_t)symbol->plt_index * (uint64_t)plt->entry_size;
  uint64_t entry_address =
      got_address + (uint64_t)symbol->got_index * (uint64_t)got->entry_size;
  uint64_t trampoline_address = plt_address + offset;
  uint8_t* p = (uint8_t*)(plt_buffer->value + offset);

  int32_t got_disp = (int32_t)(entry_address - (trampoline_address + 6));
  int32_t plt_disp = (int32_t)(plt_address - (trampoline_address + 16));

  memcpy(p + 2, &got_disp, 4);
  *(uint32_t*)(p + 7) = (uint32_t)(symbol->plt_index - 1);
  memcpy(p + 12, &plt_disp, 4);
}

static void CheckOptions(Linker* linker) {
  (void)linker;
}

LinkerArchitecture* NewX86_64LinkerArchitecture(void) {
  LinkerArchitecture* arch = malloc(sizeof(LinkerArchitecture));
  arch->code_start_address = CodeStartAddress;
  arch->data_start_address = DataStartAddress;
  arch->machine_type = ELF_MACHINE_TYPE_X86_64;
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
