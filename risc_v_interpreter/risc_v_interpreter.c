//
//  risc_v_interpreter.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_interpreter.h"
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include "risc_v_disassembler.h"

void RISCVInterpreterDumpRegisters(RISCVInterpreter* interpreter) {
  const int kWidth = 40;
  for (int i = 0; i < RV_NUM_INT_REGS; i += 2) {
    DisassemblePrintRegister(stdout, i, kRegTypeInt, "");

    int n = printf("  0x%016" PRIx64 " (%" PRId64 ")", interpreter->iregs[i],
           interpreter->iregs[i]);
    n = kWidth - n;
    while (n-- > 0) {
      putchar(' ');
    }
    DisassemblePrintRegister(stdout, i+1, kRegTypeInt, "");
    printf("  0x%016" PRIx64 " (%" PRId64 ")", interpreter->iregs[i+1],
           interpreter->iregs[i+1]);
    printf("\n");
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i += 2) {
    DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
    int n =  printf("  %g  ", interpreter->fregs[i]);
    n = kWidth - n;
    while (n-- > 0) {
      putchar(' ');
    }
    DisassemblePrintRegister(stdout, i+1, kRegTypeFloat, "");
    printf("  %g", interpreter->fregs[i+1]);
    printf("\n");
  }
  printf("pc: 0x%" PRIx64 " (%" PRId64 ")\n", interpreter->pc, interpreter->pc);
}

static void DumpStateAndExit(RISCVInterpreter* interpreter) {
  RISCVInterpreterDumpRegisters(interpreter);
  exit(1);
}

// On entry:
// t0: address of resolver data structure.  This is called "link_map" in
//     Linux, but in this loader it's the address of the LoadedDynamicLibrary
//     that contains the GOT entry being resolved.
// t1: the byte offset from the start of the GOTPLT (the part of the GOT
//     that contains addresses of functions rather than data) to the GOT
//     entry that is being resolved.
//
// The procedure is to use the offset into the GOTPLT to find a relocation
// for the GOT entry.  This gives us an ELFSymbol, from which we get the
// symbol name.  This is looked up in the dynamic symbol tables and if
// found, the GOT entry is set to the address of the symbol and the PC
// is set to that address.
static void ResolveAndFixupSymbol(RISCVInterpreter* interpreter) {
  const int t0 = 5;
  const int t1 = 6;
  int64_t offset = interpreter->iregs[t1];
  LoadedDynamicLibrary* lib = (LoadedDynamicLibrary*)interpreter->iregs[t0];
  const void* relocs =
      DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(jmprel));
  if (relocs == NULL) {
    fprintf(stderr, "Failed to find relocations in library %s\n",
            lib->filename.value);
    exit(1);
  }
  // The offset is a byte offset into the .got.plt.  We need to find the
  // relocation which will be at index offset/8 into the relocation table.
  // TODO: is it safe to assume that relocations are in order in the
  // .rela.plt table or do we need to search for the offset?
  int64_t reloc_index = offset / 8;

  const ELFRelocation* reloc = (const ELFRelocation*)relocs + reloc_index;
  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  // printf("Resolving symbol %s\n", sym_name);
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t symbol_address = found_lib->load_address + symbol->value;

  // Fixup GOT entry to contain the symbol address.
  *(uint64_t*)(lib->load_address + reloc->offset) = symbol_address;

  // Finally jump to the address.  The PC will be incremented after the
  // ecall instruction so we need to set it to one instruction before
  // the address we want - 4 bytes.
  interpreter->pc = symbol_address - 4;
}

const bool kDumpRegsonEbreak = false;
const bool kShowRegChanges = false;

static void HandleEcall(RISCVInterpreter* interpreter) {
  switch (interpreter->iregs[REG(t6)]) {
    case RISC_V_ECALL_HALT:
      exit(0);
      break;
    case RISC_V_ECALL_OPEN: {
      const char* filename = (const char*)interpreter->iregs[REG(a1)];
      int mode = (int)interpreter->iregs[REG(a2)];
      interpreter->iregs[REG(a0)] = open(filename, mode);
      break;
    }
    case RISC_V_ECALL_CLOSE: {
      int fd = (int)interpreter->iregs[REG(a1)];
      interpreter->iregs[REG(a0)] = close(fd);
      break;
    }
    case RISC_V_ECALL_READ: {
      int fd = (int)interpreter->iregs[REG(a1)];
      void* addr = (void*)interpreter->iregs[REG(a2)];
      size_t size = (size_t)interpreter->iregs[REG(a3)];
      interpreter->iregs[REG(a0)] = read(fd, addr, size);
      break;
    }
    case RISC_V_ECALL_WRITE: {
      int fd = (int)interpreter->iregs[REG(a1)];
      const void* addr = (void*)interpreter->iregs[REG(a2)];
      size_t size = (size_t)interpreter->iregs[REG(a3)];
      interpreter->iregs[REG(a0)] = write(fd, addr, size);
      break;
    }
    case RISC_V_ECALL_LSEEK: {
      int fd = (int)interpreter->iregs[REG(a1)];
      off_t pos = (off_t)interpreter->iregs[REG(a2)];
      int whence = (int)interpreter->iregs[REG(a3)];
      interpreter->iregs[REG(a0)] = lseek(fd, pos, whence);
      break;
    }

    case RISC_V_ECALL_RESOLVE: {
      ResolveAndFixupSymbol(interpreter);
      break;
    }
    case RISC_V_ECALL_MALLOC: {
      size_t size = (size_t)interpreter->iregs[REG(a1)];
      void** addr = (void**)interpreter->iregs[REG(a2)];
      *addr = malloc(size);
      interpreter->iregs[REG(a0)] = *addr != NULL;
      break;
    }
    case RISC_V_ECALL_REALLOC: {
      void* old_addr = (void*)interpreter->iregs[REG(a1)];
      size_t size = (size_t)interpreter->iregs[REG(a2)];
      void** addr = (void**)interpreter->iregs[REG(a3)];
      *addr = realloc(old_addr, size);
      interpreter->iregs[REG(a0)] = *addr != NULL;
      break;
    }
    case RISC_V_ECALL_FREE: {
      void* addr = (void*)interpreter->iregs[REG(a1)];
      free(addr);
      break;
    }
    case RISC_V_ECALL_ABORT: {
      abort();
      break;
    }
    default:
      DumpStateAndExit(interpreter);
  }
}

static void HandleEbreak(RISCVInterpreter* interpreter) {
  if (kDumpRegsonEbreak) {
    RISCVInterpreterDumpRegisters(interpreter);
  }
  longjmp(interpreter->debugger, 1);
}

static void DumpRegChanges(RISCVInterpreter* interpreter) {
  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    if (interpreter->iregs[i] != interpreter->old_iregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeInt, "");
      printf(": %08" PRIx64 " -> %08" PRIx64 "\n", interpreter->old_iregs[i],
             interpreter->iregs[i]);
    }
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    if (interpreter->fregs[i] != interpreter->old_fregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
      printf(": %g -> %g\n", interpreter->old_fregs[i], interpreter->fregs[i]);
    }
  }
}

void RISCVInterpreterInit(RISCVInterpreter* interpreter, Loader* loader,
                          uint64_t entry_address, int argc,
                          char** argv, bool trace_regs,
                          bool trace_instructions) {
  memset(interpreter, 0, sizeof(RISCVInterpreter));
  interpreter->trace_regs = trace_regs;
  interpreter->trace_instructions = trace_instructions;
  interpreter->num_steps = -1;

  // Create symbol resolver code.  This is invoked from the first
  // PLT entry with the following registers set:
  // t0: address of resolver data in the GOT.
  // t1: index into PLT for function to be called.
  //
  // The resolver data in the GOT contains:
  // [0]: Address of this code.
  // [1]: Address of LoadedDynamicLibrary containing the GOT and PLT
  //
  // This loads x10 with the ecall opcode (RISC_V_ECALL_RESOLVE)
  // and invokes an ecall instruction.
  interpreter->symbol_resolver_code[0] =
      RV_OPCODE(op_imm) | (31 << 7) |
      (RISC_V_ECALL_RESOLVE << 20);                          // addi x31, x0, 6
  interpreter->symbol_resolver_code[1] = RV_OPCODE(system);  // ecall

  interpreter->loader = loader;
  interpreter->stack = malloc(RISC_V_STACK_SIZE);
  interpreter->iregs[RV_SP_REG] =
      (int64_t)(interpreter->stack + RISC_V_STACK_SIZE);

  int64_t* iregs = interpreter->iregs;

  // Invoke interpreter at startup code.  This will call main and then
  // halt.
  int32_t* startup = interpreter->startup_code;
  // Startup code is:
  // x1 = entry_address
  // jalr x1, x1, 0
  // mv x10, 1
  // ecall
  startup[0] = RV_OPCODE(jalr) | (1 << 7) | (1 << 15);     // jalr x1, x1 ,0
  startup[1] = RV_OPCODE(op_imm) | (31 << 7) | (1 << 20);  // addi x31, x0, 1
  startup[2] = RV_OPCODE(system);                          // ecall
  interpreter->iregs[1] = entry_address;
  interpreter->pc = (int64_t)startup;

  // Move argc and argv into regs a0 and a1.
  iregs[RV_INT_ARG_START] = argc;
  iregs[RV_INT_ARG_START + 1] = (int64_t)argv;

  if (kShowRegChanges) {
    interpreter->trace_regs = true;
  }
}

void RISCVInterpreterCycle(RISCVInterpreter* interpreter) {
  int64_t* iregs = interpreter->iregs;
  double* fregs = interpreter->fregs;
  for (; interpreter->pc != 0; interpreter->pc += 4) {
    if (interpreter->num_steps > 0) {
      --interpreter->num_steps;
    }

    if (interpreter->trace_instructions) {
      interpreter->current_symbol =
          LoaderFindSymbolAndCacheResult(interpreter->loader, interpreter->pc);
    }

    // x0 is hardcoded as zero.  Reset it every loop in case it's been
    // overwritten.
    iregs[RV_INT_ZERO_REG] = 0;

    if (interpreter->trace_regs) {
      memcpy(interpreter->old_iregs, interpreter->iregs,
             sizeof(interpreter->iregs));
      memcpy(interpreter->old_fregs, interpreter->fregs,
             sizeof(interpreter->fregs));
    }
    // Disassemble unless it's an ebreak instruction.
    bool is_ebreak = *(int32_t*)interpreter->pc == ((1 << 20) | 0x73);
    if (!is_ebreak && interpreter->trace_instructions) {
      DisassembleRiscVInstruction(interpreter, (void*)interpreter->pc, stdout);
    }

    int32_t inst = *(uint32_t*)interpreter->pc;  // Signed 32 bits.
    RVInstOpcode opcode = inst & 0x7f;
    int rd = (inst >> 7) & 0x1f;
    int rs1 = (inst >> 15) & 0x1f;
    int rs2 = (inst >> 20) & 0x1f;
    switch (opcode) {
      case RV_OPCODE(op): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        if (funct7 == RV_F7(mul)) {
          switch (funct3) {
            case RV_F3(mul):
              iregs[rd] = iregs[rs1] * iregs[rs2];
              break;
            case RV_F3(mulh): {
              // From:
              // https://stackoverflow.com/questions/28868367/getting-the-high-part-of-64-bit-integer-multiplication
              int64_t a = iregs[rs1];
              int64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(mulhsu): {
              int64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(mulhu): {
              uint64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(div):
              iregs[rd] = iregs[rs1] / iregs[rs2];
              break;
            case RV_F3(divu):
              iregs[rd] = iregs[rs1] / iregs[rs2];
              break;
            case RV_F3(rem):
              iregs[rd] = iregs[rs1] % iregs[rs2];
              break;
            case RV_F3(remu):
              iregs[rd] = iregs[rs1] % iregs[rs2];
              break;
          }
          break;
        }
        switch (funct3) {
          case RV_F3(add):  // add and sub:
            if (funct7 == RV_F7(sub)) {
              iregs[rd] = iregs[rs1] - iregs[rs2];
            } else {
              iregs[rd] = iregs[rs1] + iregs[rs2];
            }
            break;
          case RV_F3(sll):
            iregs[rd] = iregs[rs1] << iregs[rs2];
            break;
          case RV_F3(slt):
            iregs[rd] = iregs[rs1] < iregs[rs2];
            break;
          case RV_F3(sltu):
            iregs[rd] = (uint64_t)iregs[rs1] < (uint64_t)iregs[rs2];
            break;
          case RV_F3 (xor):
            iregs[rd] = iregs[rs1] ^ iregs[rs2];
            break;
          case RV_F3(srl):  // and sra
            if (funct7 == RV_F7(sra)) {
              iregs[rd] = iregs[rs1] >> iregs[rs2];
            } else {
              iregs[rd] = (uint64_t)iregs[rs1] >> iregs[rs2];
            }
            break;

          case RV_F3(or):
            iregs[rd] = iregs[rs1] | iregs[rs2];
            break;
          case RV_F3(and):
            iregs[rd] = iregs[rs1] & iregs[rs2];
            break;
          default:
            break;
        }
        break;
      }
      case RV_OPCODE(op_imm): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        switch (funct3) {
          case RV_F3(addi):
            iregs[rd] = iregs[rs1] + immed;
            break;
          case RV_F3(slti):
            iregs[rd] = iregs[rs1] < immed;
            break;
          case RV_F3(sltiu):
            iregs[rd] = (uint64_t)iregs[rs1] < (uint64_t)immed;
            break;
          case RV_F3(xori):
            iregs[rd] = iregs[rs1] ^ immed;
            break;
          case RV_F3(ori):
            iregs[rd] = iregs[rs1] | immed;
            break;
          case RV_F3(andi):
            iregs[rd] = iregs[rs1] & immed;
            break;
          case RV_F3(slli): {
            int shamt = (inst >> 20) & 0x3f;  // 6 bits in R64
            iregs[rd] = iregs[rs1] << shamt;
            break;
          }
          case RV_F3(srli): {
            // In R64 the shift amount is 6 bits and overlaps the F7 field
            // by one bit.  We need to clear this bottom bit before checking
            // the F7.
            int funct7 = (inst >> 25) & 0x7e;  // Bottom bit is cleared
            int shamt = (inst >> 20) & 0x3f;
            if (funct7 == RV_F7(srai)) {
              iregs[rd] = iregs[rs1] >> shamt;
            } else {
              iregs[rd] = (uint64_t)iregs[rs1] >> shamt;
            }
            break;
          }
          default:
            break;
        }
        break;
      }
      case RV_OPCODE(lui): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
        iregs[rd] = immed << 12;
        break;
      }
      case RV_OPCODE(auipc): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
        iregs[rd] = interpreter->pc + (immed << 12);
        break;
      }
      case RV_OPCODE(jal): {
        // immediate at bit 12 is encoded as imm[20|10:1|11|19:12]
        int64_t imm = inst >> 12;  // Auto sign extended to 64 bits.
        int64_t immed = (imm & 0xff) << 12 | ((imm >> 8) & 1) << 11 |
                        ((imm >> 9) & 0x3ff) << 1 | ((imm >> 19) & 1) << 20;
        // Sign extend to 64 bits.
        immed <<= 63 - 20;
        immed >>= 63 - 20;
        iregs[rd] = interpreter->pc + 4;
        interpreter->pc += immed - 4;
        break;
      }
      case RV_OPCODE(jalr): {
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        int64_t old_pc = interpreter->pc;
        interpreter->pc = iregs[rs1] + immed - 4;
        iregs[rd] = old_pc + 4;
        break;
      }
      case RV_OPCODE(branch): {
        int64_t hi = inst >> 25;
        int64_t offset = (rd & 0x1e) | ((rd & 1) << 11) | ((hi & 0x3f) << 5) |
                         ((hi >> 6) << 12);
        offset -= 4;  // Adjust for += 4 at end of loop.
        int funct3 = (inst >> 12) & 0x7;
        switch (funct3) {
          case RV_F3(beq):
            if (iregs[rs1] == iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bne):
            if (iregs[rs1] != iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(blt):
            if (iregs[rs1] < iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bge):
            if (iregs[rs1] >= iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bltu):
            if ((uint64_t)iregs[rs1] < (uint64_t)iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bgeu):
            if ((uint64_t)iregs[rs1] >= (uint64_t)iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
        }
        break;
      }
      case RV_OPCODE(load): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        switch (funct3) {
          case RV_F3(lb):
            iregs[rd] = *(int8_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(lh):
            iregs[rd] = *(int16_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(lw):
            iregs[rd] = *(int32_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(lbu):
            iregs[rd] = *(uint8_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(lhu):
            iregs[rd] = *(uint16_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(lwu):
            iregs[rd] = *(uint32_t*)(iregs[rs1] + immed);
            break;
          case RV_F3(ld):
            iregs[rd] = *(uint64_t*)(iregs[rs1] + immed);
            break;
        }
        break;
      }
      case RV_OPCODE(store): {
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
        int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
        switch (funct3) {
          case RV_F3(sb):
            *(int8_t*)(iregs[rs1] + immed) = iregs[rs2];
            break;
          case RV_F3(sh):
            *(int16_t*)(iregs[rs1] + immed) = iregs[rs2];
            break;
          case RV_F3(sw):
            *(int32_t*)(iregs[rs1] + immed) = (int32_t)iregs[rs2];
            break;
          case RV_F3(sd):
            *(uint64_t*)(iregs[rs1] + immed) = iregs[rs2];
            break;
        }
        break;
      }
      case RV_OPCODE(misc_mem): {
        break;
      }
      case RV_OPCODE(system): {
        int op = inst >> 20;
        switch (op) {
          case 0:  // ecall
            HandleEcall(interpreter);
            break;
          case 1:  // ebreak
            HandleEbreak(interpreter);
            break;
        }
        break;
      }
      case RV_OPCODE(op_imm_32): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        switch (funct3) {
          case RV_F3(addiw):
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) + immed;
            break;
          case RV_F3(slliw): {
            int shamt = (inst >> 20) & 0x3f;
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) << shamt;
            break;
          }
          case RV_F3(srliw): {
            int funct7 = (inst >> 25) & 0x7f;
            int shamt = (inst >> 20) & 0x3f;
            if (funct7 == RV_F7(sraiw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) >> shamt;
            } else {
              iregs[rd] = (uint32_t)(iregs[rs1] & 0xffffffff) >> shamt;
            }
          }
        }
        break;
      }
      case RV_OPCODE(op_32): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        switch (funct3) {
          case RV_F3(addw):
            if (funct7 == RV_F7(subw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) -
                          (int32_t)(iregs[rs2] & 0xffffffff);
            } else {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) +
                          (int32_t)(iregs[rs2] & 0xffffffff);
            }
            break;
          case RV_F3(sllw):
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) << iregs[rs2];
            break;

          case RV_F3(srlw):
            if (funct7 == RV_F7(sraiw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) >> iregs[rs2];
            } else {
              iregs[rd] = (uint32_t)(iregs[rs1] & 0xffffffff) >> iregs[rs2];
            }
            break;
        }
        break;
      }
      case RV_OPCODE(load_fp): {
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        if (funct3 == RV_F3(flw)) {
          fregs[rd] = *(float*)(iregs[rs1] + immed);
        } else {
          fregs[rd] = *(double*)(iregs[rs1] + immed);
        }
        break;
      }
      case RV_OPCODE(store_fp): {
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
        int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
        if (funct3 == RV_F3(fsw)) {
          *(float*)(iregs[rs1] + immed) = (float)fregs[rs2];
        } else {
          *(double*)(iregs[rs1] + immed) = fregs[rs2];
        }
        break;
      }

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define MAX(x, y) (x) > (y) ? (x) : (y)

      // TODO: these don't use rounding mode.
      case RV_OPCODE(op_fp): {
        int rm = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        switch (funct7) {
          case RV_F7(fadd_s):
            fregs[rd] = (float)fregs[rs1] + (float)fregs[rs2];
            break;
          case RV_F7(fsub_s):
            fregs[rd] = (float)fregs[rs1] - (float)fregs[rs2];
            break;
          case RV_F7(fmul_s):
            fregs[rd] = (float)fregs[rs1] * (float)fregs[rs2];
            break;
          case RV_F7(fdiv_s):
            fregs[rd] = (float)fregs[rs1] / (float)fregs[rs2];
            break;
          case RV_F7(fsqrt_s):
            fregs[rd] = (float)sqrt((float)fregs[rs1]);
            break;
          case RV_F7(fmin_s):
            if (rm == RV_F3(fmin_s)) {
              fregs[rd] = MIN((float)fregs[rs1], (float)fregs[rs2]);
            } else {
              fregs[rd] = MIN((float)fregs[rs1], (float)fregs[rs2]);
            }
            break;
          case RV_F7(fadd_d):
            fregs[rd] = fregs[rs1] + fregs[rs2];
            break;
          case RV_F7(fsub_d):
            fregs[rd] = fregs[rs1] - fregs[rs2];
            break;
          case RV_F7(fmul_d):
            fregs[rd] = fregs[rs1] * fregs[rs2];
            break;
          case RV_F7(fdiv_d):
            fregs[rd] = fregs[rs1] / fregs[rs2];
            break;
          case RV_F7(fsqrt_d):
            fregs[rd] = sqrt(fregs[rs1]);
            break;
          case RV_F7(fmin_d):
            if (rm == RV_F3(fmin_s)) {
              fregs[rd] = MIN(fregs[rs1], fregs[rs2]);
            } else {
              fregs[rd] = MIN(fregs[rs1], fregs[rs2]);
            }
            break;

          // Floating point comparisons.
          case RV_F7(feq_s):  // And flt.s, fle.s
            switch (rm) {
              case RV_F3(feq_s:)
                iregs[rd] = (float)fregs[rs1] == (float)fregs[rs2];
                break;
              case RV_F3(flt_s:)
                iregs[rd] = (float)fregs[rs1] < (float)fregs[rs2];
                break;
              case RV_F3(fle_s):
                iregs[rd] = (float)fregs[rs1] <= (float)fregs[rs2];
                break;
            }
            break;

          case RV_F7(feq_d):  // And flt.d, fle.d
            switch (rm) {
            case RV_F3(feq_d:)
              iregs[rd] = fregs[rs1] == fregs[rs2];
              break;
            case RV_F3(flt_d:)
              iregs[rd] = fregs[rs1] < fregs[rs2];
              break;
            case RV_F3(fle_d):
              iregs[rd] = fregs[rs1] <= fregs[rs2];
              break;
            }
            break;

          // Moves.
          case RV_F7(fmv_w_x): {
            uint32_t bits = (uint32_t)iregs[rs1];
            float value;
            memcpy(&value, &bits, sizeof(value));
            fregs[rd] = value;
            break;
          }

          case RV_F7(fmv_x_w): {
            float value = (float)fregs[rs1];
            uint32_t bits;
            memcpy(&bits, &value, sizeof(bits));
            iregs[rd] = (int32_t)bits;
            break;
          }

          case RV_F7(fmv_d_x):
            *(int64_t*)(&fregs[rd]) = iregs[rs1];
            break;

          case RV_F7(fmv_x_d):
            iregs[rd] = *(int64_t*)(&fregs[rs1]);
            break;

          case RV_F7(fcvt_s_w):  //  and RV_F7(fcvt_s_wu):
            switch (rs2) {
              case 0:
                fregs[rd] = (float)((int32_t)iregs[rs1]);
                break;
              case 1:
                fregs[rd] = (float)((uint32_t)iregs[rs1]);
                break;
              case 2:
                fregs[rd] = (float)((int64_t)iregs[rs1]);
                break;
              case 3:
                fregs[rd] = (float)((uint64_t)iregs[rs1]);
                break;
            }
            break;

          case RV_F7(fcvt_d_w):  // and RV_F7(fcvt_d_wu)/fcvt_d_l/fcvt_d_lu:
            switch (rs2) {
              case 0:
                fregs[rd] = (double)((int32_t)iregs[rs1]);
                break;
              case 1:
                fregs[rd] = (double)((uint32_t)iregs[rs1]);
                break;
              case 2:
                fregs[rd] = (double)((int64_t)iregs[rs1]);
                break;
              case 3:
                fregs[rd] = (double)((uint64_t)iregs[rs1]);
                break;
            }
            break;

          case RV_F7(fcvt_w_s):  // and RV_F7(fcvt_wu_s):
            switch (rs2) {
              case 0:
                iregs[rd] = (int32_t)fregs[rs1];
                break;
              case 1:
                iregs[rd] = (uint32_t)fregs[rs1];
                break;
              case 2:
                iregs[rd] = (int64_t)fregs[rs1];
                break;
              case 3:
                iregs[rd] = (uint64_t)fregs[rs1];
                break;
            }
            break;

          case RV_F7(fcvt_w_d):  // and RV_F7(fcvt_wu_d)/fcvt_l_d/fcvt_lu_d:
            switch (rs2) {
              case 0:
                iregs[rd] = (int32_t)fregs[rs1];
                break;
              case 1:
                iregs[rd] = (uint32_t)fregs[rs1];
                break;
              case 2:
                iregs[rd] = (int64_t)fregs[rs1];
                break;
              case 3:
                iregs[rd] = (uint64_t)fregs[rs1];
                break;
            }
            break;

          case RV_F7(fcvt_s_d):
            fregs[rd] = (float)fregs[rs1];
            break;

          case RV_F7(fcvt_d_s):
            fregs[rd] = (float)fregs[rs1];
            break;

          // TODO: These don't use rs2.
          case RV_F7(fsgnj_s):  // All sign injection instructions.
            switch (rm) {
              case RV_F3(fsgnj_s):
                fregs[rd] = fregs[rs1];
                break;
              case RV_F3(fsgnjn_s):
                fregs[rd] = -fregs[rs1];
                break;
              case RV_F3(fsgnjx_s):
                fregs[rd] = fabs(fregs[rs1]);
                break;
            }
            break;

          case RV_F7(fsgnj_d):  // All sign injection instructions.
            switch (rm) {
              case RV_F3(fsgnj_d):
                fregs[rd] = fregs[rs1];
                break;
              case RV_F3(fsgnjn_d):
                fregs[rd] = -fregs[rs1];
                break;
              case RV_F3(fsgnjx_d):
                fregs[rd] = fabs(fregs[rs1]);
                break;
            }
            break;
        }
        break;
      }
      default:
        break;
    }

    if (interpreter->trace_regs) {
      DumpRegChanges(interpreter);
    }
    
    if (interpreter->num_steps == 0) {
      interpreter->pc += 4;
      break;
    }
  }
}

void RISCVInterpreterDestruct(RISCVInterpreter* interpreter) {
  free(interpreter->stack);
}
