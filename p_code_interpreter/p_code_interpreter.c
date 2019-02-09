//
//  p_code_interpreter.c
//  p_code_interpreter
//
//  Created by David Allison on 1/22/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "p_code_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "p_code_disassembler.h"

#define DEST(inst) ((inst >> 16) & 0xff)
#define SRC1(inst) ((inst >> 8) & 0xff)
#define SRC2(inst) (inst & 0xff)

static void DumpStateAndExit(Interpreter* interpreter) {
  // TODO: dump registers.
  exit(1);
}

static bool disassemble = false;

static void EscapeHandler(Interpreter* interpreter, int32_t code){
  switch (code) {
    case P_CODE_ESC_UNDEF_INST:
      printf("Undefined instruction opcode\n");
      DumpStateAndExit(interpreter);
      break;
    case P_CODE_ESC_DIV_ZERO:
      printf("Division by zero\n");
      DumpStateAndExit(interpreter);
      break;
    case P_CODE_ESC_WRITE: {
      // r0 = file descriptor
      // r1 = buffer
      // r2 = length
      char* buf = (char*)interpreter->iregs[1];
      interpreter->iregs[0] = write((int)interpreter->iregs[0], buf, interpreter->iregs[2]);
      break;
    }
    case P_CODE_ESC_READ: {
      // r0 = file descriptor
      // r1 = buffer
      // r2 = length
      char* buf = (char*)interpreter->iregs[1];
      interpreter->iregs[0] = read((int)interpreter->iregs[0], buf, interpreter->iregs[2]);
      break;
    }
    case P_CODE_ESC_HALT:
      exit(0);
      break;

    case P_CODE_ESC_DEBUG:
      printf("Debug escape\n");
      break;

    default:
      printf("Undefined escape\n");
      DumpStateAndExit(interpreter);
  }
}

void InterpreterInit(Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv) {
  memset(interpreter, 0, sizeof(Interpreter));
  interpreter->stack = malloc(P_CODE_STACK_SIZE);
  interpreter->iregs[PCODE_SP_REG] = (int64_t)(interpreter->stack + P_CODE_STACK_SIZE);
  interpreter->escape = EscapeHandler;
  interpreter->loader = loader;
  
  int64_t* iregs = interpreter->iregs;

  // Build a sequence of code to call main followed by esc #4. The ret instruction
  // at the end of main will return to the esc #4 instruction.
  int32_t* startup = interpreter->startup_code;
  interpreter->iregs[PCODE_PC_REG] = (int64_t)startup;
  startup[0] = 0xc3000000;                                 // call
  int64_t pcrel = entry_address - (int64_t)startup + 4;
  startup[1] = (uint32_t)(pcrel & 0xffffffffLL);           // main low word.
  startup[2] = (uint32_t)(pcrel >> 32);                    // main high word.
  startup[3] = 0x40000004;                                 // esc #4

  // Invoke interpreter at startup code.  This will call main and then
  // halt.

  // Push argv and argc onto stack.
  iregs[PCODE_SP_REG] -= 8;
  *((uint64_t*)iregs[PCODE_SP_REG]) = (int64_t)argv;
  iregs[PCODE_SP_REG] -= 4;
  *((int32_t*)iregs[PCODE_SP_REG]) = argc;
}

void InterpreterRun(Interpreter* interpreter) {
  int64_t* iregs = interpreter->iregs;
  float* fregs = interpreter->fregs;
  double* dregs = interpreter->dregs;

  for (;;) {
    // Fetch instruction from current PC location.
    // We keep a local copy of the program counter as a pointer for
    // convenience.  This is only valid in this loop and the main program counter
    // register is canonical.
    int32_t* pc = (int32_t*)iregs[PCODE_PC_REG];
    
    if (disassemble) {
      DisassemblePCodeInstruction(interpreter, pc, stdout);
    }
    
    // Fetch first word and advance PC to next word.  All instructions are at least
    // 32 bits long.
    uint32_t inst = *pc++;
    iregs[PCODE_PC_REG] += 4;

    // The top 2 bits of the first instruction word tell us the size of the
    // instruction as follows:
    //
    // Bits 31 and 30:
    // 0x   - 32 bit, with 7 bit opcode.
    // 10   - 64 bit, with 6 bit opcode.
    // 11   - 96 bit, with 6 bit opcode.

    bool is_32_bit = (inst & 0x80000000) == 0;
    if (is_32_bit) {
      // 7 bit opcode
      switch ((inst >> 24) & 0x7f) {
        case OP(add):
          iregs[DEST(inst)] = iregs[SRC1(inst)] + iregs[SRC2(inst)];
          break;
        case OP(sub):
          iregs[DEST(inst)] = iregs[SRC1(inst)] - iregs[SRC2(inst)];
          break;
        case OP(addf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] - fregs[SRC2(inst)];
          break;
        case OP(addd):
          dregs[DEST(inst)] = dregs[SRC1(inst)] - dregs[SRC2(inst)];
          break;
        case OP(subf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] - fregs[SRC2(inst)];
          break;
        case OP(subd):
          dregs[DEST(inst)] = dregs[SRC1(inst)] - dregs[SRC2(inst)];
          break;
        case OP(mul):
          iregs[DEST(inst)] = iregs[SRC1(inst)] * iregs[SRC2(inst)];
          break;
        case OP(mulf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] * fregs[SRC2(inst)];
          break;
        case OP(muld):
          dregs[DEST(inst)] = dregs[SRC1(inst)] * dregs[SRC2(inst)];
          break;
        case OP(div):
          if (iregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            iregs[DEST(inst)] = iregs[SRC1(inst)] / iregs[SRC2(inst)];
          }
          break;
        case OP(divu):
          if (iregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] / (uint64_t)iregs[SRC2(inst)];
          }
          break;
        case OP(divf):
          if (fregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            fregs[DEST(inst)] = fregs[SRC1(inst)] / fregs[SRC2(inst)];
          }
          break;
        case OP(divd):
          if (dregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            dregs[DEST(inst)] = dregs[SRC1(inst)] - dregs[SRC2(inst)];
          }
          break;
        case OP(mod):
          iregs[DEST(inst)] = iregs[SRC1(inst)] % iregs[SRC2(inst)];
          break;
        case OP(modu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] % (uint64_t)iregs[SRC2(inst)];
          break;
        case OP(lsr):
          iregs[DEST(inst)] = (uint64_t)(iregs[SRC1(inst)]) >> iregs[SRC2(inst)];
          break;
        case OP(asr):
          iregs[DEST(inst)] = iregs[SRC1(inst)] >> iregs[SRC2(inst)];
          break;
        case OP(lsl):
          iregs[DEST(inst)] = iregs[SRC1(inst)] << iregs[SRC2(inst)];
          break;
        case OP(or):
          iregs[DEST(inst)] = iregs[SRC1(inst)] | iregs[SRC2(inst)];
          break;
        case OP(and):
          iregs[DEST(inst)] = iregs[SRC1(inst)] & iregs[SRC2(inst)];
          break;
        case OP(xor):
          iregs[DEST(inst)] = iregs[SRC1(inst)] ^ iregs[SRC2(inst)];
          break;
        case OP(not):
          iregs[DEST(inst)] = !iregs[SRC1(inst)];
          break;
        case OP(inv):
          iregs[DEST(inst)] = ~iregs[SRC1(inst)];
          break;
        case OP(neg):
          iregs[DEST(inst)] = -iregs[SRC1(inst)];
          break;
        case OP(negf):
          fregs[DEST(inst)] = -fregs[SRC1(inst)];
          break;
        case OP(negd):
          dregs[DEST(inst)] = -dregs[SRC1(inst)];
          break;
        case OP(cmpeq):
          iregs[DEST(inst)] = iregs[SRC1(inst)] == iregs[SRC2(inst)];
          break;
        case OP(cmpne):
          iregs[DEST(inst)] = iregs[SRC1(inst)] != iregs[SRC2(inst)];
          break;
        case OP(cmplt):
          iregs[DEST(inst)] = iregs[SRC1(inst)] < iregs[SRC2(inst)];
          break;
        case OP(cmple):
          iregs[DEST(inst)] = iregs[SRC1(inst)] <= iregs[SRC2(inst)];
          break;
        case OP(cmpgt):
          iregs[DEST(inst)] = iregs[SRC1(inst)] > iregs[SRC2(inst)];
          break;
        case OP(cmpge):
          iregs[DEST(inst)] = iregs[SRC1(inst)] >= iregs[SRC2(inst)];
          break;
        case OP(cmpltu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] < (uint64_t)iregs[SRC2(inst)];
          break;
        case OP(cmpleu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] <= (uint64_t)iregs[SRC2(inst)];
          break;
        case OP(cmpgtu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] > (uint64_t)iregs[SRC2(inst)];
          break;
        case OP(cmpgeu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] >= (uint64_t)iregs[SRC2(inst)];
          break;
       case OP(cmpeqf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] == fregs[SRC2(inst)];
          break;
        case OP(cmpnef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] != fregs[SRC2(inst)];
          break;
        case OP(cmpltf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] < fregs[SRC2(inst)];
          break;
        case OP(cmplef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] <= fregs[SRC2(inst)];
          break;
        case OP(cmpgtf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] > fregs[SRC2(inst)];
          break;
        case OP(cmpgef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] >= fregs[SRC2(inst)];
          break;
        case OP(cmpeqd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] == dregs[SRC2(inst)];
          break;
        case OP(cmpned):
          iregs[DEST(inst)] = dregs[SRC1(inst)] != dregs[SRC2(inst)];
          break;
        case OP(cmpltd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] < dregs[SRC2(inst)];
          break;
        case OP(cmpled):
          iregs[DEST(inst)] = dregs[SRC1(inst)] <= dregs[SRC2(inst)];
          break;
        case OP(cmpgtd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] < dregs[SRC2(inst)];
          break;
        case OP(cmpged):
          iregs[DEST(inst)] = dregs[SRC1(inst)] >= dregs[SRC2(inst)];
          break;
        case OP(decsp):
          iregs[PCODE_SP_REG] -= inst & 0xffffff;
          break;
        case OP(incsp):
          iregs[PCODE_SP_REG] += inst & 0xffffff;
          break;
        case OP(push):
          iregs[PCODE_SP_REG] -= 4;
          *((int32_t*)iregs[PCODE_SP_REG]) = (int32_t)iregs[DEST(inst)];
          break;
        case OP(pushf):
          iregs[PCODE_SP_REG] -= 4;
          *((float*)iregs[PCODE_SP_REG]) = fregs[DEST(inst)];
          break;
        case OP(pushd):
          iregs[PCODE_SP_REG] -= 8;
         *((double*)iregs[PCODE_SP_REG]) = dregs[DEST(inst)];
          break;
        case OP(pushx):
          iregs[PCODE_SP_REG] -= 8;
          *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[DEST(inst)];
          break;
        case OP(pop):
          iregs[DEST(inst)] = *((int32_t*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 4;
          break;
        case OP(popf):
          fregs[DEST(inst)] = *((float*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 4;
          break;
        case OP(popd):
          dregs[DEST(inst)] = *((double*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          break;
        case OP(popx):
          iregs[DEST(inst)] = *((uint64_t*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          break;
        case OP(mov):
          iregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case OP(movf):
          fregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case OP(movd):
          dregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case OP(ret):
          // Return from function.  sp[0] contains 64 bit return address;
          iregs[PCODE_PC_REG] = *((uint64_t*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->iregs[PCODE_PC_REG]);
          break;

        case OP(cbra):
          // TODO
          break;
        case OP(i2f):
          fregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case OP(i2d):
          dregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case OP(ui2f):
          fregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
          break;
        case OP(ui2d):
          dregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
          break;
        case OP(f2d):
          dregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case OP(d2f):
          fregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case OP(f2i):
          iregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case OP(d2i):
          iregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case OP(f2ui):
          iregs[DEST(inst)] = (uint64_t)fregs[SRC1(inst)];
          break;
        case OP(d2ui):
          iregs[DEST(inst)] = (uint64_t)dregs[SRC1(inst)];
          break;
       case OP(rcall):
          iregs[PCODE_SP_REG] -= 8;
          *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[PCODE_PC_REG] + 8;
          iregs[PCODE_PC_REG] = iregs[DEST(inst)];
          interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->iregs[PCODE_PC_REG]);
          break;

        case OP(esc):
          // Call the escape function with the immediate value.
          if (interpreter->escape != NULL) {
            interpreter->escape(interpreter, inst & 0xffffff);
          }
          break;
        default:
          interpreter->escape(interpreter, P_CODE_ESC_UNDEF_INST);
          break;
      }
    } else {
      // 64 or 96 bit instruction.
      bool is_64_bit = (inst & 0x40000000) == 0;
      if (is_64_bit) {
        // 64 bit instructions.
        // 6 bit opcode
        switch ((inst >> 24) & 0x3f) {
        case OP(ldw):
          iregs[DEST(inst)] = *(int32_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldh):
          iregs[DEST(inst)] = *(int16_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldb):
          iregs[DEST(inst)] = *(int8_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(lduw):
          iregs[DEST(inst)] = *(uint32_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldub):
          iregs[DEST(inst)] = *(uint8_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(lduh):
          iregs[DEST(inst)] = *(uint16_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldx):
          iregs[DEST(inst)] = *(uint64_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldf):
          fregs[DEST(inst)] = *(float*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(ldd):
          dregs[DEST(inst)] = *(double*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(stw):
          *(int32_t*)(iregs[SRC1(inst)] + *pc) = (int32_t)iregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(sth):
          *(int16_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(stx):
          *(uint64_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(stf):
          *(float*)(iregs[SRC1(inst)] + *pc) = fregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(std):
          *(double*)(iregs[SRC1(inst)] + *pc) = dregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(stb):
          *(int8_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;

        case OP(movc):
          iregs[DEST(inst)] = *pc;
          iregs[PCODE_PC_REG] += 4;
          break;
        case OP(movfc):
          fregs[DEST(inst)] = *pc++;
          iregs[PCODE_PC_REG] += 4;
          break;

        case OP(bz):
          if (iregs[DEST(inst)] == 0) {
            // Relative branch.
            iregs[PCODE_PC_REG] += (int32_t)*pc - 4;
          } else {
            iregs[PCODE_PC_REG] += 4;
          }
          break;
        case OP(bnz):
          if (iregs[DEST(inst)] != 0) {
            // Relative branch.
            iregs[PCODE_PC_REG] += (int32_t)*pc - 4;
          } else {
            iregs[PCODE_PC_REG] += 4;
          }
          break;
        case OP(bra):
            // Relative branch.
          iregs[PCODE_PC_REG] += (int32_t)*pc - 4;
          break;
        case OP(addc):
          iregs[DEST(inst)] = iregs[SRC1(inst)] + *pc;
          iregs[PCODE_PC_REG] += 4;
          break;

        default:
          interpreter->escape(interpreter, P_CODE_ESC_UNDEF_INST);
          break;
        }
      } else {
        // 96 bit instructions.
        // 6 bit opcode.
        switch ((inst >> 24) & 0x3f) {
         case OP(movdc):
          dregs[DEST(inst)] = *(double*)pc;
          iregs[PCODE_PC_REG] += 8;
          break;
        case OP(movxc):
          iregs[DEST(inst)] = *(uint64_t*)pc;
          iregs[PCODE_PC_REG] += 8;
          break;
        case OP(jmp):
          iregs[PCODE_PC_REG] = *(uint64_t*)pc + iregs[PCODE_PC_REG];
          break;
        case OP(call):
          iregs[PCODE_SP_REG] -= 8;
          *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[PCODE_PC_REG] + 8;
          iregs[PCODE_PC_REG] = *(uint64_t*)pc + iregs[PCODE_PC_REG] + 8;
          interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->iregs[PCODE_PC_REG]);
          break;
          case OP(cjmp): {
            // Load the value at the absolute address in the operand.  Then jump to that value.
            uint64_t* addr = *(uint64_t**)pc;
            iregs[PCODE_PC_REG] = *addr;
            interpreter->current_symbol = LoaderFindSymbol(interpreter->loader, interpreter->iregs[PCODE_PC_REG]);
          break;
          }
        case OP(adr): {
          // Operand is offset from PC to address.
          uint64_t addr = *(uint64_t*)pc + iregs[PCODE_PC_REG] + 8;
          iregs[DEST(inst)] = addr;
          break;
          }
        default:
          interpreter->escape(interpreter, P_CODE_ESC_UNDEF_INST);
          break;
       }
      }
    }
  }
}

void InterpreterDestruct(Interpreter* interpreter) {
  free(interpreter->stack);
}
