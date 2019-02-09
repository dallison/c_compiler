//
//  linker_reloc.c
//  linker
//
//  Created by David Allison on 1/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_reloc.h"
#include "linker_file.h"
#include "linker.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "risc_v_machine.h"
#include "linker_dynamic.h"

LinkerRelocation* NewLinkerRelocation(const char* symbol_name, int64_t offset,
                                      int32_t reloc_type) {
  LinkerRelocation* reloc = malloc(sizeof(LinkerRelocation));
  StringInit(&reloc->symbol_name, symbol_name);
  reloc->offset = offset;
  reloc->type = reloc_type;
  reloc->addend = 0;
  reloc->section = NULL;
  reloc->got_offset = -1;
  reloc->plt_offset = -1;
  return reloc;
}

void LinkerRelocationDestruct(LinkerRelocation* reloc) {
  StringDestruct(&reloc->symbol_name);
}

void LinkerRelocationDelete(LinkerRelocation* reloc) {
  LinkerRelocationDestruct(reloc);
  free(reloc);
}

// Read a relocation from the ELF file and add it to the linker's
// relocation table.  A relocation is a modification to a piece of
// data in the ELF file.  Once the linker has determined the addresses
// of all symbols it can use those symbol values to modify the requested
// locations.  Typically these are references to external symbols whose
// addresses are not known at compile time.
void LinkerReadRelocation(Linker* linker,
                           LinkerFile* file,
                           ELFReaderFile* elf_file,
                           ELFRelocation* reloc,
                           const char* symbol_table_address,
                           ELFReaderSection* reloc_section,
                           ELFReaderSection* symtab,
                           ELFReaderSection* strtab) {
  int32_t symbol_index = ELF_R_SYM(reloc->info);
  int32_t reloc_type = ELF_R_TYPE(reloc->info);
  ELFSymbol* elf_sym = (ELFSymbol*)(symbol_table_address + symbol_index * symtab->header->entsize);

  // Create a LinkerRelocation object.
  LinkerRelocation* linker_reloc =
  NewLinkerRelocation((const char*)strtab->contents + elf_sym->name,
                      reloc->offset, reloc_type);

  // The section that this reloc applies to is encoded in the name.
  // Remove the ".rela" or ".rel" prefix to give the name of the
  // section to which it applies.
  String target_section_name;
  if (StringStartsWith(&reloc_section->name, ".rela")) {
    StringInit(&target_section_name, reloc_section->name.value + 5);
  } else if (StringStartsWith(&reloc_section->name, ".rel")) {
    StringInit(&target_section_name, reloc_section->name.value + 4);
  } else {
    // Unknown relocation section name, can't handle it.
    LinkerWarning(file, "bad-rel-section",
                  "Unknown relocation section name %s",
                  reloc_section->name.value);
    LinkerRelocationDelete(linker_reloc);
    return;
  }

  // Find the target section by name.
  ELFReaderSection* target = LinkerFileFindSection(file, &target_section_name);
  if (target != NULL) {
    linker_reloc->section = target;
  } else {
    LinkerWarning(file, "unknown-rel-section",
                  "Cannot find section %s to apply relocations",
                  target_section_name.value);
  }

  // Add relocation to file relocations vector.
  VectorAppend(&file->relocations, linker_reloc);

  StringDestruct(&target_section_name);
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
  SetBitField32(target_address, 12, 20, encoded_value);
  
}

// Find the relocation associated with pc.  This is used to handle
// the R_RISCV_PCREL_LO12_X relocations that refer to an auipc instruction with
// a R_RISCV_PCREL_HI20 relocation.
static LinkerRelocation* FindRelocation(Linker* linker, int64_t pc) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    LinkerFile* file = linker->files.value[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      LinkerRelocation* reloc = file->relocations.value[reloc_index];
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
  ((immed >> 1) & 0x3ff) << 21 |   // imm[10:1]
  ((immed >> 11) & 1) << 20 |    // imm[11]
  ((immed >> 12) & 0xff) << 12 |      // imm[19:12]
  rd << 7 |                           // rd
  opcode;                         // opcode
}

// Apply a relocation.  A relocation targets a particular instruction or
// data in a section.
static void ApplyRelocation(Linker* linker, LinkerFile* file,
                            LinkerRelocation* reloc) {
  ELFReaderSection* target_section = reloc->section;
  if (target_section == NULL) {
    return;
  }
  
  // Find the value of the symbol to use.  This looks in the local symbol
  // table first, then the global symbol table.
  LinkerSymbol* symbol = LinkerFileFindSymbol(file, reloc->symbol_name.value);
  if (symbol == NULL) {
    // Trying to apply relocation for undefined symbol
    LinkerError(file, "Undefined symbol %s used in relocation", reloc->symbol_name.value);
    return;
  }

  // What address are we applying the relocation to.
  char* target_address = (char*)target_section->contents + reloc->offset;
  
  uint64_t S = symbol->address;   // Symbol address.
  int64_t A = reloc->addend;      // Addend.
  
  // PC in P-Code is address of next instruction.  All the PC relative
  // instructions are 96 bits long.
  uint64_t P = target_section->address + reloc->offset + 12;

  printf("Applying relocation type %d for symbol %s(0x%llx) to offset %lld\n",
         reloc->type,
         reloc->symbol_name.value,
         symbol->address, reloc->offset);
  switch (linker->elf_machine_type) {
    case ELF_MACHINE_TYPE_PCODE:        // P-CODE
      switch (reloc->type) {
        case R_PCODE_MOVXC: {
          // Instruction is 96 bits long.  The relocation is applied
          // to the second and third word, in little endian format.
          // The instruction can be either a movxc or an adr.  In the latter
          // case the value stored is pc relative.
          int opcode = target_address[3] & 0x3f;
          if (opcode == 1) {    // MOVXC
            *((uint64_t*)(target_address + 4)) = S + A;
          } else if (opcode == 5) {   // ADR
            *((uint64_t*)(target_address + 4)) = S + A - P;
          } else {
            LinkerError(file, "Unsupported P-CODE MOVXC relocation opcode %d", opcode);
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
          // Symbol contains a got_offset that is the offset into the
          // PLT.  The instruction will be an CALL
          // instruction that contains the offset relative to the current
          // PC.
          uint64_t plt_address = linker->dynamic_section->
                procedure_linkage_table.address;
          
          // Each PLT entry is 36 bytes long.
          uint64_t addr = plt_address + symbol->plt_offset * 36 + A;
          *((uint64_t*)(target_address + 4)) = addr - P;
          break;
        }
        case R_PCODE_GOT_ENTRY: {
          // Instruction is 96 bits long.  The relocation is applied
          // to the second and third word, in little endian format.
          // Symbol contains a got_offset that is the offset into the
          // global offset table.  The instruction will be an ADR
          // instruction that contains the offset relative to the current
          // PC.
          uint64_t got_address = linker->dynamic_section->
                global_offset_table.address;
          // Each GOT entry is 8 bytes long.
          uint64_t addr = got_address + symbol->got_offset * 8 + A;
          *((uint64_t*)(target_address + 4)) = addr - P;
          break;
        }
        case R_PCODE_GOT_DATA:
          break;
        case R_PCODE_GOT_FUNC:
          break;
        default:
          LinkerError(file, "Unsupported P-CODE relocation type %d", reloc->type);
          break;
      }
      break;
      
    case ELF_MACHINE_TYPE_RISC_V: {      // RISC-V
      // See https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
      // for details on the relocation calculations.
      uint64_t S = symbol->address;
      int64_t A = reloc->addend;
      uint64_t P = target_section->address + reloc->offset;
      int32_t hi20 = ((int32_t)S + 0x800) >> 12;
      int32_t lo12 = (int32_t)S - hi20;

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
          hi20 = (offset + 0x800) >> 12;
          lo12 = (int32_t)(S - P) - hi20;
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
          hi20 = (offset + 0x800) >> 12;
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
          LinkerRelocation* hi20_reloc = FindRelocation(linker, S);
          if (hi20_reloc == NULL) {
            LinkerError(file, "Unable to locate R_RISCV_PCREL_HI20 relocation");
            return;
          }
          
          // Get the symbol from the hi20 relocation.
          symbol = LinkerFileFindSymbol(file, hi20_reloc->symbol_name.value);
          if (symbol == NULL) {
            // Trying to apply relocation for undefined symbol
            LinkerError(file, "Undefined symbol %s", hi20_reloc->symbol_name.value);
            return;
          }
          
          // Difference in PC to auipc instruction.
          int32_t pcdiff = (int32_t)(reloc->offset - hi20_reloc->offset);
          S = symbol->address;
          
          int32_t offset = (int32_t)(S - P + pcdiff - A);
          hi20 = (offset + 0x800) >> 12;
          lo12 = (int32_t)(S - P + pcdiff) - hi20;
          if (reloc->type == R_RISCV_PCREL_LO12_S) {
            SetBitField32(target_address, 7, 5, lo12);
            SetBitField32(target_address, 25, 7, lo12 >> 5);
          } else {
            SetBitField32(target_address, 20, 12, lo12);
          }
          return;
        }
          // TODO:
        case R_RISCV_CALL_PLT:
          return;
        case R_RISCV_GOT_HI20:
          return;
       case R_RISCV_HI20:
          SetBitField32(target_address, 12, 20, hi20);
          return;
        case R_RISCV_LO12_I:
          SetBitField32(target_address, 20, 12, lo12);
          return;
        case R_RISCV_LO12_S:
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
      break;
    }
    default:
      LinkerError(file, "Unsupported ELF machine type %d", linker->elf_machine_type);

  }

}

// Go through each file (each .o file) and apply the relocations contained
// in it to their target sections.
void LinkerApplyAllRelocations(Linker* linker) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    LinkerFile* file = linker->files.value[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      ApplyRelocation(linker, file, file->relocations.value[reloc_index]);
    }
  }
}

