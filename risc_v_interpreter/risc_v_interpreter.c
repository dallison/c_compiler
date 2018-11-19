//
//  risc_v_interpreter.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "risc_v_disassembler.h"


static void DumpRegisters(Interpreter* interpreter) {
  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    DisassemblePrintRegister(stdout, i, kRegTypeInt, "");
    
    printf("  0x%016llx (%lld)\n", interpreter->iregs[i], interpreter->iregs[i]);
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
    printf("  %g\n", interpreter->fregs[i]);
   }
  printf("pc: 0x%llx (%lld)\n", interpreter->pc, interpreter->pc);
}

static void DumpStateAndExit(Interpreter* interpreter) {
  DumpRegisters(interpreter);
  exit(1);
}

const bool kDisassemble = false;
const bool kDumpRegsonEbreak = false;
const bool kShowRegChanges = false;

static void HandleEcall(Interpreter* interpreter) {
  switch (interpreter->iregs[10]) {
    case RISC_V_ECALL_HALT:
      exit(0);
      break;
    case RISC_V_ECALL_OPEN: {
      const char* filename = (const char*)interpreter->iregs[11];
      int mode = (int)interpreter->iregs[12];
      interpreter->iregs[10] = open(filename, mode);
      break;
    }
    case RISC_V_ECALL_CLOSE: {
      int fd = (int)interpreter->iregs[11];
      interpreter->iregs[10] = close(fd);
      break;
    }
    case RISC_V_ECALL_READ: {
      int fd = (int)interpreter->iregs[11];
      void* addr = (void*)interpreter->iregs[12];
      size_t size = (size_t)interpreter->iregs[13];
      interpreter->iregs[10] = read(fd, addr, size);
      break;
    }
    case RISC_V_ECALL_WRITE: {
      int fd = (int)interpreter->iregs[11];
      const void* addr = (void*)interpreter->iregs[12];
      size_t size = (size_t)interpreter->iregs[13];
      interpreter->iregs[10] = write(fd, addr, size);
      break;
    }
    default:
      DumpStateAndExit(interpreter);
 }
}

static void HandleEbreak(Interpreter* interpreter) {
  if (kDumpRegsonEbreak) {
    DumpRegisters(interpreter);
  }
}

static void DumpRegChanges(Interpreter* interpreter) {
  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    if (interpreter->iregs[i] != interpreter->old_iregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeInt, "");
      printf(": %08llx -> %08llx\n", interpreter->old_iregs[i], interpreter->iregs[i]);
    }
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    if (interpreter->fregs[i] != interpreter->old_fregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
      printf(": %g -> %g\n", interpreter->old_fregs[i], interpreter->fregs[i]);
    }
  }
}

void InterpreterInit(Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv) {
  memset(interpreter, 0, sizeof(Interpreter));
  interpreter->loader = loader;
  interpreter->stack = malloc(RISC_V_STACK_SIZE);
  interpreter->iregs[RV_SP_REG] = (int64_t)(interpreter->stack + RISC_V_STACK_SIZE);
    
  int64_t* iregs = interpreter->iregs;
  
  // Invoke interpreter at startup code.  This will call main and then
  // halt.
  int32_t* startup = interpreter->startup_code;
  // Startup code is:
  // x1 = entry_address
  // jalr x1, x1, 0
  // mv x10, 1
  // ecall
  startup[0] = RV_OPCODE(jalr) | (1 << 7) | (1 << 15); // jalr x1, x1 ,0
  startup[1] = RV_OPCODE(op_imm) | (10 << 7) | (1 << 20);    // addi x10, x0, 1
  startup[2] = RV_OPCODE(system);                           // ecall
  interpreter->iregs[1] = entry_address;
  interpreter->pc = (int64_t)startup;
  
  // Move argc and argv into regs a0 and a1.
  iregs[RV_INT_ARG_START] = argc;
  iregs[RV_INT_ARG_START+1] = (int64_t)argv;
  
  interpreter->trace_regs = kShowRegChanges;
}

void InterpreterRun(Interpreter* interpreter) {
  int64_t* iregs = interpreter->iregs;
  double* fregs = interpreter->fregs;

  interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->pc);
  
  for (; interpreter->pc != 0; interpreter->pc += 4) {
    // x0 is hardcoded as zero.  Reset it every loop in case it's been
    // overwritten.
    iregs[RV_INT_ZERO_REG] = 0;
    
    if (interpreter->trace_regs) {
      memcpy(interpreter->old_iregs, interpreter->iregs, sizeof(interpreter->iregs));
      memcpy(interpreter->old_fregs, interpreter->fregs, sizeof(interpreter->fregs));
    }
    if (kDisassemble) {
      DisassembleRiscVInstruction(interpreter, (void*)interpreter->pc, stdout);
    }
    
    int32_t inst = *(uint32_t*)interpreter->pc;     // Signed 32 bits.
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
              uint64_t    a_lo = (uint32_t)a;
              uint64_t    a_hi = a >> 32;
              uint64_t    b_lo = (uint32_t)b;
              uint64_t    b_hi = b >> 32;
              
              uint64_t    a_x_b_hi =  a_hi * b_hi;
              uint64_t    a_x_b_mid = a_hi * b_lo;
              uint64_t    b_x_a_mid = b_hi * a_lo;
              uint64_t    a_x_b_lo =  a_lo * b_lo;
              
              uint64_t    carry_bit = ((uint64_t)(uint32_t)a_x_b_mid +
                                       (uint64_t)(uint32_t)b_x_a_mid +
                                       (a_x_b_lo >> 32) ) >> 32;
              
              uint64_t    multhi = a_x_b_hi +
              (a_x_b_mid >> 32) + (b_x_a_mid >> 32) +
              carry_bit;
              
              iregs[rd] =  multhi;
              break;
            }
            case RV_F3(mulhsu): {
              int64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t    a_lo = (uint32_t)a;
              uint64_t    a_hi = a >> 32;
              uint64_t    b_lo = (uint32_t)b;
              uint64_t    b_hi = b >> 32;
              
              uint64_t    a_x_b_hi =  a_hi * b_hi;
              uint64_t    a_x_b_mid = a_hi * b_lo;
              uint64_t    b_x_a_mid = b_hi * a_lo;
              uint64_t    a_x_b_lo =  a_lo * b_lo;
              
              uint64_t    carry_bit = ((uint64_t)(uint32_t)a_x_b_mid +
                                       (uint64_t)(uint32_t)b_x_a_mid +
                                       (a_x_b_lo >> 32) ) >> 32;
              
              uint64_t    multhi = a_x_b_hi +
              (a_x_b_mid >> 32) + (b_x_a_mid >> 32) +
              carry_bit;
              
              iregs[rd] =  multhi;
              break;
            }
            case RV_F3(mulhu): {
              uint64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t    a_lo = (uint32_t)a;
              uint64_t    a_hi = a >> 32;
              uint64_t    b_lo = (uint32_t)b;
              uint64_t    b_hi = b >> 32;
              
              uint64_t    a_x_b_hi =  a_hi * b_hi;
              uint64_t    a_x_b_mid = a_hi * b_lo;
              uint64_t    b_x_a_mid = b_hi * a_lo;
              uint64_t    a_x_b_lo =  a_lo * b_lo;
              
              uint64_t    carry_bit = ((uint64_t)(uint32_t)a_x_b_mid +
                                       (uint64_t)(uint32_t)b_x_a_mid +
                                       (a_x_b_lo >> 32) ) >> 32;
              
              uint64_t    multhi = a_x_b_hi +
              (a_x_b_mid >> 32) + (b_x_a_mid >> 32) +
              carry_bit;
              
              iregs[rd] =  multhi;
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
          case RV_F3(xor):
            iregs[rd] = iregs[rs1] ^ iregs[rs2];
            break;
          case RV_F3(srl):     // and sra
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
        int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
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
            int shamt = (inst >> 20) & 0x3f;    // 6 bits in R64
            iregs[rd] = iregs[rs1] << shamt;
            break;
          }
          case RV_F3(srli): {
            // In R64 the shift amount is 6 bits and overlaps the F7 field
            // by one bit.  We need to clear this bottom bit before checking
            // the F7.
            int funct7 = (inst >> 25) & 0x7e; // Bottom bit is cleared
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
        int64_t immed = inst >> 12;     // Auto sign extended to 64 bits.
        iregs[rd] = immed << 12;
        break;
      }
      case RV_OPCODE(auipc): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int64_t immed = inst >> 12;     // Auto sign extended to 64 bits.
        iregs[rd] = interpreter->pc + (immed << 12);
        break;
      }
      case RV_OPCODE(jal): {
        // immediate at bit 12 is encoded as imm[20|10:1|11|19:12]
        int64_t imm = inst >> 12;     // Auto sign extended to 64 bits.
        int64_t immed = (imm & 0xff) << 12 | ((imm >> 8) & 1) << 11 |
            ((imm >> 9) & 0x3ff) << 1 |
            ((imm >> 19) & 1) << 20;
        // Sign extend to 64 bits.
        immed <<= 63-20;
        immed >>= 63-20;
        iregs[rd] = interpreter->pc + 4;
        interpreter->pc += immed - 4;
        if (rd != 0) {
          // A jal with x0 as the rd is a 'j' instruction and that jumps to the same
          // procedure.
          interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->pc+4);
        }
        break;
      }
      case RV_OPCODE(jalr): {
        int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
        int64_t old_pc = interpreter->pc;
        interpreter->pc = iregs[rs1] + immed - 4;
        interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->pc+4);
        iregs[rd] = old_pc + 4;
        break;
      }
      case RV_OPCODE(branch): {
        int64_t hi = inst >> 25;
        int64_t offset = (rd & 0x1e) | ((rd & 1) << 11) |
            ((hi & 0x3f) << 5) | ((hi >> 6) << 12);
        offset -= 4;      // Adjust for += 4 at end of loop.
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
        int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
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
        int64_t immed_hi = inst >> 25;     // Auto sign extended to 64 bits.
        int64_t immed = immed_hi << 5 | rd;   // rd is the low 5 bits of offset.
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
          case 0:     // ecall
            HandleEcall(interpreter);
            break;
          case 1:    // ebreak
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
        int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
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
        break;
      }
      case RV_OPCODE(store_fp): {
        break;
      }
      case RV_OPCODE(op_fp): {
        break;
      }
      default:
        break;
    }
    
    if (interpreter->trace_regs) {
      DumpRegChanges(interpreter);
    }
  }
}

void InterpreterDestruct(Interpreter* interpreter) {
  free(interpreter->stack);
}

