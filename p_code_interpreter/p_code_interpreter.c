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
#include "elf.h"
#include "loader_lifecycle.h"
#include "p_code_disassembler.h"
#include "p_code_syscalls.h"

#define DEST(inst) ((inst >> 16) & 0xff)
#define SRC1(inst) ((inst >> 8) & 0xff)
#define SRC2(inst) (inst & 0xff)

static void DumpStateAndExit(PCodeInterpreter* interpreter) {
  // TODO: dump registers.
  exit(1);
}

static void EscapeHandler(PCodeInterpreter* interpreter, int32_t code);

static bool disassemble = false;

void PCodeInterpreterSetDisassemble(bool enabled) {
  disassemble = enabled;
}

static uint16_t ReadU16(const void* p) {
  uint16_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static uint32_t ReadU32(const void* p) {
  uint32_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static uint64_t ReadU64(const void* p) {
  uint64_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static float ReadFloat(const void* p) {
  float value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static double ReadDouble(const void* p) {
  double value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static void WriteU16(void* p, uint16_t value) {
  memcpy(p, &value, sizeof(value));
}

static void WriteU32(void* p, uint32_t value) {
  memcpy(p, &value, sizeof(value));
}

static void WriteU64(void* p, uint64_t value) {
  memcpy(p, &value, sizeof(value));
}

static void WriteFloat(void* p, float value) {
  memcpy(p, &value, sizeof(value));
}

static void WriteDouble(void* p, double value) {
  memcpy(p, &value, sizeof(value));
}

// Resolve a PLT symbol and fixup the GOT entry.
// On entry:
// t1 (r26): contains the index into the relocation table that
//           refers to the symbol to resolve and the offset
//           into the library of the GOT entry.
// t2 (r27): contains the address of the resolver data inside
//           the GOT.  This consists of a single pointer containing
//           the address of the LoadedDynamicLibrary to use for
//           the resolution.
//
// The relocations for the PLTGOT are in the DT(jmprel) entry
// in the dynamic section, which can be obtained from the
// library.
static void ResolveAndFixupSymbol(PCodeInterpreter* interpreter) {
  uint64_t reloc_index = interpreter->iregs[26];
  uint64_t* resolver_data = (uint64_t*)interpreter->iregs[27];
  LoadedDynamicLibrary* lib = (LoadedDynamicLibrary*)resolver_data[0];
  const void* relocs = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(jmprel));
  if (relocs == NULL) {
    fprintf(stderr, "Failed to find relocations in library %s\n", lib->filename.value);
    exit(1);
  }
  const ELFRelocation* reloc = (const ELFRelocation*)relocs + reloc_index;
  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  printf("Resolving symbol %s\n", sym_name);
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries,
                                    sym_name, &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t symbol_address = found_lib->load_address + symbol->value;
  
  // Fixup GOT entry to contain the symbol address.
  *(uint64_t*)(lib->load_address + reloc->offset) = symbol_address;
  
  // Finally jump to the address.
  interpreter->iregs[PCODE_PC_REG] = symbol_address;
}

static void EscapeHandler(PCodeInterpreter* interpreter, int32_t code){
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
    case P_CODE_ESC_EXIT:
      exit((int)interpreter->iregs[0]);
      break;
    case P_CODE_ESC_EXIT_CLEAN:
      if (interpreter->loader != NULL) {
        LoaderLifecycleMarkExecutableFiniComplete(
            interpreter->loader, interpreter->loader->lifecycle);
      }
      interpreter->exit_code = (int)interpreter->iregs[0];
      interpreter->running = false;
      break;
    case P_CODE_ESC_ABORT:
      abort();
      break;
    case P_CODE_ESC_PROGRAM_RETURN:
      interpreter->exit_code = (int)interpreter->iregs[0];
      interpreter->running = false;
      break;

    case P_CODE_ESC_DEBUG:
      printf("Debug escape\n");
      break;

    case P_CODE_ESC_RESOLVE:
      ResolveAndFixupSymbol(interpreter);
      break;

    case P_CODE_ESC_SYSCALL:
      interpreter->iregs[0] = PCodeHandlePackedSyscall(
          interpreter, interpreter->iregs[0],
          (const void*)(uintptr_t)interpreter->iregs[1]);
      break;
      
    default:
      printf("Undefined escape\n");
      DumpStateAndExit(interpreter);
  }
}

void PCodeInterpreterInit(PCodeInterpreter* interpreter) {
  memset(interpreter, 0, sizeof(PCodeInterpreter));
  interpreter->running = true;
  interpreter->escape = EscapeHandler;
  
  // Create symbol resolver code.  This is invoked from the first
  // PLT entry with the following registers set:
  // t1: index into PLT for function to be called.
  // t2: address of resolver data in the GOT.
  //
  // The resolver data in the GOT contains:
  // [0]: Address of LoadedDynamicLibrary containing the GOT and PLT
  // [1]: Address of this code.
  //
  // So we get two registers containing the information we need to find
  // the library and the function to resolve.  The library comes from
  // the 8-byte word in t2.  The address of the .dynamic section can
  // be obtained from the library.  From that we can get the DT(jmprel)
  // relocations, each of which contains the symbol for the function to
  // be resolved.
  //
  // For this trampoline we just need to invoke esc #6 (P_CODE_ESC_RESOLVE).
  // This will resolve the symbol, write its address into the GOT
  // determined by the relocation and then jump to the symbol, thus
  // invoking the function.
  interpreter->symbol_resolver_code[0] = PCODE_OP(esc) << 24 |
      P_CODE_ESC_RESOLVE;
}

static void PCodeInterpreterPrepareCall(PCodeInterpreter* interpreter,
                                        uint64_t fn) {
  interpreter->iregs[PCODE_SP_REG] =
      (int64_t)(interpreter->stack + P_CODE_STACK_SIZE);
  interpreter->iregs[PCODE_SP_REG] -= 8;
  WriteU64((void*)interpreter->iregs[PCODE_SP_REG], 0);
  interpreter->iregs[PCODE_PC_REG] = (int64_t)fn;
}

typedef struct {
  int64_t iregs[PCODE_NUM_INT_REGS];
  float fregs[PCODE_NUM_FLOAT_REGS];
  double dregs[PCODE_NUM_DOUBLE_REGS];
  bool running;
  int exit_code;
} PCodeSavedState;

static void PCodeInterpreterSaveState(PCodeInterpreter* interpreter,
                                      PCodeSavedState* saved) {
  memcpy(saved->iregs, interpreter->iregs, sizeof(saved->iregs));
  memcpy(saved->fregs, interpreter->fregs, sizeof(saved->fregs));
  memcpy(saved->dregs, interpreter->dregs, sizeof(saved->dregs));
  saved->running = interpreter->running;
  saved->exit_code = interpreter->exit_code;
}

static void PCodeInterpreterRestoreState(PCodeInterpreter* interpreter,
                                         const PCodeSavedState* saved) {
  memcpy(interpreter->iregs, saved->iregs, sizeof(interpreter->iregs));
  memcpy(interpreter->fregs, saved->fregs, sizeof(interpreter->fregs));
  memcpy(interpreter->dregs, saved->dregs, sizeof(interpreter->dregs));
  interpreter->running = saved->running;
  interpreter->exit_code = saved->exit_code;
}

static void PCodeInterpreterStep(PCodeInterpreter* interpreter) {
  int64_t* iregs = interpreter->iregs;
  float* fregs = interpreter->fregs;
  double* dregs = interpreter->dregs;

  int32_t* pc = (int32_t*)iregs[PCODE_PC_REG];
  interpreter->current_symbol = LoaderFindSymbolAndCacheResult(interpreter->loader,
                                                 interpreter->iregs[PCODE_PC_REG]);

  if (disassemble) {
    DisassemblePCodeInstruction(interpreter, pc, stdout);
    fflush(stdout);
  }

  uint32_t inst = *pc++;
  iregs[PCODE_PC_REG] += 4;

  bool is_32_bit = (inst & 0x80000000) == 0;
  if (is_32_bit) {
    switch ((inst >> 24) & 0x7f) {
        case PCODE_OP(add):
          iregs[DEST(inst)] = iregs[SRC1(inst)] + iregs[SRC2(inst)];
          break;
        case PCODE_OP(sub):
          iregs[DEST(inst)] = iregs[SRC1(inst)] - iregs[SRC2(inst)];
          break;
        case PCODE_OP(addf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] + fregs[SRC2(inst)];
          break;
        case PCODE_OP(addd):
          dregs[DEST(inst)] = dregs[SRC1(inst)] + dregs[SRC2(inst)];
          break;
        case PCODE_OP(subf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] - fregs[SRC2(inst)];
          break;
        case PCODE_OP(subd):
          dregs[DEST(inst)] = dregs[SRC1(inst)] - dregs[SRC2(inst)];
          break;
        case PCODE_OP(mul):
          iregs[DEST(inst)] = iregs[SRC1(inst)] * iregs[SRC2(inst)];
          break;
        case PCODE_OP(mulf):
          fregs[DEST(inst)] = fregs[SRC1(inst)] * fregs[SRC2(inst)];
          break;
        case PCODE_OP(muld):
          dregs[DEST(inst)] = dregs[SRC1(inst)] * dregs[SRC2(inst)];
          break;
        case PCODE_OP(div):
          if (iregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            iregs[DEST(inst)] =
                (uint64_t)((int64_t)iregs[SRC1(inst)] /
                           (int64_t)iregs[SRC2(inst)]);
          }
          break;
        case PCODE_OP(divu):
          if (iregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] / (uint64_t)iregs[SRC2(inst)];
          }
          break;
        case PCODE_OP(divf):
          if (fregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            fregs[DEST(inst)] = fregs[SRC1(inst)] / fregs[SRC2(inst)];
          }
          break;
        case PCODE_OP(divd):
          if (dregs[SRC2(inst)] == 0) {
            interpreter->escape(interpreter, P_CODE_ESC_DIV_ZERO);
          } else {
            dregs[DEST(inst)] = dregs[SRC1(inst)] / dregs[SRC2(inst)];
          }
          break;
        case PCODE_OP(mod):
          iregs[DEST(inst)] =
              (uint64_t)((int64_t)iregs[SRC1(inst)] %
                         (int64_t)iregs[SRC2(inst)]);
          break;
        case PCODE_OP(modu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] % (uint64_t)iregs[SRC2(inst)];
          break;
        case PCODE_OP(lsr):
          iregs[DEST(inst)] = (uint64_t)(iregs[SRC1(inst)]) >> iregs[SRC2(inst)];
          break;
        case PCODE_OP(asr):
          iregs[DEST(inst)] = iregs[SRC1(inst)] >> iregs[SRC2(inst)];
          break;
        case PCODE_OP(lsl):
          iregs[DEST(inst)] = iregs[SRC1(inst)] << iregs[SRC2(inst)];
          break;
        case PCODE_OP(or):
          iregs[DEST(inst)] = iregs[SRC1(inst)] | iregs[SRC2(inst)];
          break;
        case PCODE_OP(and):
          iregs[DEST(inst)] = iregs[SRC1(inst)] & iregs[SRC2(inst)];
          break;
        case PCODE_OP(xor):
          iregs[DEST(inst)] = iregs[SRC1(inst)] ^ iregs[SRC2(inst)];
          break;
        case PCODE_OP(not):
          iregs[DEST(inst)] = !iregs[SRC1(inst)];
          break;
        case PCODE_OP(inv):
          iregs[DEST(inst)] = ~iregs[SRC1(inst)];
          break;
        case PCODE_OP(neg):
          iregs[DEST(inst)] = -iregs[SRC1(inst)];
          break;
        case PCODE_OP(negf):
          fregs[DEST(inst)] = -fregs[SRC1(inst)];
          break;
        case PCODE_OP(negd):
          dregs[DEST(inst)] = -dregs[SRC1(inst)];
          break;
        case PCODE_OP(cmpeq):
          iregs[DEST(inst)] = iregs[SRC1(inst)] == iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpne):
          iregs[DEST(inst)] = iregs[SRC1(inst)] != iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmplt):
          iregs[DEST(inst)] = iregs[SRC1(inst)] < iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmple):
          iregs[DEST(inst)] = iregs[SRC1(inst)] <= iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgt):
          iregs[DEST(inst)] = iregs[SRC1(inst)] > iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpge):
          iregs[DEST(inst)] = iregs[SRC1(inst)] >= iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpltu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] < (uint64_t)iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpleu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] <= (uint64_t)iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgtu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] > (uint64_t)iregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgeu):
          iregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)] >= (uint64_t)iregs[SRC2(inst)];
          break;
       case PCODE_OP(cmpeqf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] == fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpnef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] != fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpltf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] < fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmplef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] <= fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgtf):
          iregs[DEST(inst)] = fregs[SRC1(inst)] > fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgef):
          iregs[DEST(inst)] = fregs[SRC1(inst)] >= fregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpeqd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] == dregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpned):
          iregs[DEST(inst)] = dregs[SRC1(inst)] != dregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpltd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] < dregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpled):
          iregs[DEST(inst)] = dregs[SRC1(inst)] <= dregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpgtd):
          iregs[DEST(inst)] = dregs[SRC1(inst)] > dregs[SRC2(inst)];
          break;
        case PCODE_OP(cmpged):
          iregs[DEST(inst)] = dregs[SRC1(inst)] >= dregs[SRC2(inst)];
          break;
        case PCODE_OP(decsp):
          iregs[PCODE_SP_REG] -= inst & 0xffffff;
          break;
        case PCODE_OP(incsp):
          iregs[PCODE_SP_REG] += inst & 0xffffff;
          break;
        case PCODE_OP(push):
          iregs[PCODE_SP_REG] -= 4;
          WriteU32((void*)iregs[PCODE_SP_REG], (uint32_t)iregs[DEST(inst)]);
          break;
        case PCODE_OP(pushf):
          iregs[PCODE_SP_REG] -= 4;
          WriteFloat((void*)iregs[PCODE_SP_REG], fregs[DEST(inst)]);
          break;
        case PCODE_OP(pushd):
          iregs[PCODE_SP_REG] -= 8;
          WriteDouble((void*)iregs[PCODE_SP_REG], dregs[DEST(inst)]);
          break;
        case PCODE_OP(pushx):
          iregs[PCODE_SP_REG] -= 8;
          WriteU64((void*)iregs[PCODE_SP_REG], (uint64_t)iregs[DEST(inst)]);
          break;
        case PCODE_OP(pop):
          iregs[DEST(inst)] = (int32_t)ReadU32((void*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 4;
          break;
        case PCODE_OP(popf):
          fregs[DEST(inst)] = ReadFloat((void*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 4;
          break;
        case PCODE_OP(popd):
          dregs[DEST(inst)] = ReadDouble((void*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          break;
        case PCODE_OP(popx):
          iregs[DEST(inst)] = ReadU64((void*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          break;
        case PCODE_OP(mov):
          iregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case PCODE_OP(movf):
          fregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case PCODE_OP(movd):
          dregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case PCODE_OP(ret):
          // Return from function.  sp[0] contains 64 bit return address;
          iregs[PCODE_PC_REG] = ReadU64((void*)iregs[PCODE_SP_REG]);
          iregs[PCODE_SP_REG] += 8;
          break;

        case PCODE_OP(cbra):
          // TODO
          break;
        case PCODE_OP(i2f):
          fregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case PCODE_OP(i2d):
          dregs[DEST(inst)] = iregs[SRC1(inst)];
          break;
        case PCODE_OP(ui2f):
          fregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
          break;
        case PCODE_OP(ui2d):
          dregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
          break;
        case PCODE_OP(f2d):
          dregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case PCODE_OP(d2f):
          fregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case PCODE_OP(f2i):
          iregs[DEST(inst)] = fregs[SRC1(inst)];
          break;
        case PCODE_OP(d2i):
          iregs[DEST(inst)] = dregs[SRC1(inst)];
          break;
        case PCODE_OP(f2ui):
          iregs[DEST(inst)] = (uint64_t)fregs[SRC1(inst)];
          break;
        case PCODE_OP(d2ui):
          iregs[DEST(inst)] = (uint64_t)dregs[SRC1(inst)];
          break;
       case PCODE_OP(rcall):
          iregs[PCODE_SP_REG] -= 8;
          WriteU64((void*)iregs[PCODE_SP_REG], (uint64_t)iregs[PCODE_PC_REG]);
          iregs[PCODE_PC_REG] = iregs[DEST(inst)];
          break;

        case PCODE_OP(esc):
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
        case PCODE_OP(ldw):
          iregs[DEST(inst)] = (int32_t)ReadU32((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldh):
          iregs[DEST(inst)] = (int16_t)ReadU16((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldb):
          iregs[DEST(inst)] = *(int8_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(lduw):
          iregs[DEST(inst)] = ReadU32((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldub):
          iregs[DEST(inst)] = *(uint8_t*)(iregs[SRC1(inst)] + *pc);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(lduh):
          iregs[DEST(inst)] = ReadU16((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldx):
          iregs[DEST(inst)] = ReadU64((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldf):
          fregs[DEST(inst)] = ReadFloat((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(ldd):
          dregs[DEST(inst)] = ReadDouble((void*)(iregs[SRC1(inst)] + *pc));
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(stw):
          WriteU32((void*)(iregs[SRC1(inst)] + *pc), (uint32_t)iregs[DEST(inst)]);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(sth):
          WriteU16((void*)(iregs[SRC1(inst)] + *pc), (uint16_t)iregs[DEST(inst)]);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(stx):
          WriteU64((void*)(iregs[SRC1(inst)] + *pc), (uint64_t)iregs[DEST(inst)]);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(stf):
          WriteFloat((void*)(iregs[SRC1(inst)] + *pc), fregs[DEST(inst)]);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(std):
          WriteDouble((void*)(iregs[SRC1(inst)] + *pc), dregs[DEST(inst)]);
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(stb):
          *(int8_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
          iregs[PCODE_PC_REG] += 4;
          break;

        case PCODE_OP(movc):
          iregs[DEST(inst)] = *pc;
          iregs[PCODE_PC_REG] += 4;
          break;
        case PCODE_OP(movfc):
          fregs[DEST(inst)] = ReadFloat(pc);
          iregs[PCODE_PC_REG] += 4;
          break;

        case PCODE_OP(bz):
          if (iregs[DEST(inst)] == 0) {
            // Relative branch.
            iregs[PCODE_PC_REG] += (int32_t)*pc + 4;
          } else {
            iregs[PCODE_PC_REG] += 4;
          }
          break;
        case PCODE_OP(bnz):
          if (iregs[DEST(inst)] != 0) {
            // Relative branch.
            iregs[PCODE_PC_REG] += (int32_t)*pc + 4;
          } else {
            iregs[PCODE_PC_REG] += 4;
          }
          break;
        case PCODE_OP(bra):
            // Relative branch.
          iregs[PCODE_PC_REG] += (int32_t)*pc + 4;
          break;
        case PCODE_OP(addc):
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
         case PCODE_OP(movdc):
          dregs[DEST(inst)] = ReadDouble(pc);
          iregs[PCODE_PC_REG] += 8;
          break;
        case PCODE_OP(movxc):
          iregs[DEST(inst)] = ReadU64(pc);
          iregs[PCODE_PC_REG] += 8;
          break;
        case PCODE_OP(jmp):
          iregs[PCODE_PC_REG] = ReadU64(pc) + iregs[PCODE_PC_REG] + 8;
          break;
        case PCODE_OP(call):
          iregs[PCODE_SP_REG] -= 8;
          WriteU64((void*)iregs[PCODE_SP_REG], (uint64_t)(iregs[PCODE_PC_REG] + 8));
          iregs[PCODE_PC_REG] = ReadU64(pc) + iregs[PCODE_PC_REG] + 8;
          break;
       case PCODE_OP(cjmp): {
            // Load the value at the pc-relative address in the operand.
            // Then jump to that value.
            uint64_t offset = ReadU64(pc);    // Offset from PC.
            iregs[PCODE_PC_REG] =
                ReadU64((void*)(iregs[PCODE_PC_REG] + 8 + offset));
          break;
          }
        case PCODE_OP(adr): {
          // Operand is offset from PC to address.
          uint64_t addr = ReadU64(pc) + iregs[PCODE_PC_REG] + 8;
          iregs[DEST(inst)] = addr;
          iregs[PCODE_PC_REG] += 8;
        break;
          }
        default:
          interpreter->escape(interpreter, P_CODE_ESC_UNDEF_INST);
          break;
       }
      }
    }
}

void PCodeInterpreterCall(PCodeInterpreter* interpreter, uint64_t fn) {
  if (interpreter == NULL || fn == 0) {
    return;
  }
  PCodeSavedState saved;
  PCodeInterpreterSaveState(interpreter, &saved);
  PCodeInterpreterPrepareCall(interpreter, fn);
  while (interpreter->iregs[PCODE_PC_REG] != 0) {
    PCodeInterpreterStep(interpreter);
  }
  PCodeInterpreterRestoreState(interpreter, &saved);
}

static bool CopyGuestArgv(PCodeInterpreter* interpreter, int argc, char** argv,
                          uint64_t* guest_argv, uint64_t* argument_bottom) {
  uint64_t stack_base = (uint64_t)(uintptr_t)interpreter->stack;
  uint64_t stack_top = (stack_base + P_CODE_STACK_SIZE) & ~0x7ULL;
  *guest_argv = 0;
  *argument_bottom = stack_top;
  if (argc <= 0 || argv == NULL) {
    return true;
  }

  size_t string_bytes = 0;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    if (len > P_CODE_STACK_SIZE - string_bytes) {
      return false;
    }
    string_bytes += len;
  }
  if ((size_t)argc >= P_CODE_STACK_SIZE / sizeof(uint64_t)) {
    return false;
  }
  size_t vector_bytes = (size_t)(argc + 1) * sizeof(uint64_t);
  if (string_bytes + vector_bytes + 7 > P_CODE_STACK_SIZE) {
    return false;
  }

  uint64_t string_address = stack_top - string_bytes;
  uint64_t vector_address = (string_address - vector_bytes) & ~0x7ULL;
  char* string_out = (char*)(uintptr_t)string_address;
  uint64_t* pointer_out = (uint64_t*)(uintptr_t)vector_address;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    memcpy(string_out, argv[i], len);
    pointer_out[i] = (uint64_t)(uintptr_t)string_out;
    string_out += len;
  }
  pointer_out[argc] = 0;
  *guest_argv = vector_address;
  *argument_bottom = vector_address;
  return true;
}

int PCodeInterpreterRun(PCodeInterpreter* interpreter, Loader* loader,
                        uint64_t entry_address, int argc, char** argv) {
  if (interpreter->stack == NULL) {
    interpreter->stack = malloc(P_CODE_STACK_SIZE);
  }
  interpreter->escape = EscapeHandler;
  interpreter->loader = loader;
  interpreter->running = true;
  interpreter->exit_code = 0;

  int64_t* iregs = interpreter->iregs;

  int32_t* startup = interpreter->startup_code;
  interpreter->iregs[PCODE_PC_REG] = (int64_t)startup;
  startup[0] = 0xc0000000 | PCODE_OP(call) << 24;
  int64_t pcrel = entry_address - (int64_t)startup - 12;
  startup[1] = (uint32_t)(pcrel & 0xffffffffLL);
  startup[2] = (uint32_t)(pcrel >> 32);
  startup[3] = PCODE_OP(esc) << 24 | P_CODE_ESC_PROGRAM_RETURN;

  uint64_t guest_argv;
  uint64_t argument_bottom;
  if (!CopyGuestArgv(interpreter, argc, argv, &guest_argv, &argument_bottom)) {
    argc = 0;
    guest_argv = 0;
    argument_bottom =
        ((uint64_t)(uintptr_t)(interpreter->stack + P_CODE_STACK_SIZE)) &
        ~0x7ULL;
  }
  iregs[PCODE_SP_REG] = (int64_t)argument_bottom;
  iregs[PCODE_SP_REG] -= 8;
  WriteU64((void*)iregs[PCODE_SP_REG], guest_argv);
  iregs[PCODE_SP_REG] -= 4;
  WriteU32((void*)iregs[PCODE_SP_REG], (uint32_t)argc);

  while (interpreter->running) {
    if (interpreter->iregs[PCODE_PC_REG] == 0) {
      interpreter->exit_code = (int)interpreter->iregs[0];
      break;
    }
    PCodeInterpreterStep(interpreter);
  }
  return interpreter->exit_code;
}

bool PCodeGuestAddressExecutable(Loader* loader, uint64_t addr) {
  if (loader == NULL || addr == 0) {
    return false;
  }
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    if (region->segment->type != PT(load) ||
        (region->segment->flags & PF(x)) == 0) {
      continue;
    }
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr < end) {
      return true;
    }
  }
  return false;
}

uint64_t PCodeLookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !PCodeGuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

void PCodeGuestCallVoidFunction(PCodeInterpreter* interpreter, uint64_t fn) {
  if (interpreter == NULL || fn == 0) {
    return;
  }
  PCodeInterpreterCall(interpreter, fn);
}

static bool PCodeLifecycleCallback(void* context, LoadedDynamicLibrary* image,
                                   uint64_t function,
                                   LoaderLifecyclePhase phase) {
  (void)image;
  (void)phase;
  typedef struct {
    Loader* loader;
    PCodeInterpreter* interpreter;
  } PCodeLifecycleContext;
  PCodeLifecycleContext* ctx = context;
  if (!PCodeGuestAddressExecutable(ctx->loader, function)) {
    LoaderError("Function array entry 0x%llx is not executable\n",
                (unsigned long long)function);
    return false;
  }
  PCodeGuestCallVoidFunction(ctx->interpreter, function);
  return true;
}

static bool RunGuestLifecyclePhase(Loader* loader, PCodeInterpreter* interpreter,
                                   LoaderLifecyclePhase phase) {
  typedef struct {
    Loader* loader;
    PCodeInterpreter* interpreter;
  } PCodeLifecycleContext;
  PCodeLifecycleContext ctx = {loader, interpreter};
  return LoaderLifecycleRunPhase(loader, loader->lifecycle, phase,
                                 PCodeLifecycleCallback, &ctx);
}

bool PCodeGuestRunInitArrays(Loader* loader, PCodeInterpreter* interpreter) {
  return RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecyclePreinit) &&
         RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecycleInit);
}

bool PCodeGuestRunFiniArrays(Loader* loader, PCodeInterpreter* interpreter) {
  return RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecycleFini);
}

void PCodeGuestRunProgramFini(Loader* loader, PCodeInterpreter* interpreter) {
  PCodeGuestCallVoidFunction(interpreter,
                             PCodeLookupGuestFunction(loader,
                                                      "__davecc_run_fini"));
}

bool PCodeGuestRunProgramShutdown(Loader* loader,
                                  PCodeInterpreter* interpreter) {
  if (!LoaderLifecycleExecutableFiniAlreadyDone(loader->lifecycle)) {
    uint64_t guest_fini = PCodeLookupGuestFunction(loader, "__davecc_run_fini");
    if (guest_fini != 0) {
      PCodeGuestCallVoidFunction(interpreter, guest_fini);
      LoaderLifecycleMarkExecutableFiniComplete(loader, loader->lifecycle);
    }
  }
  return PCodeGuestRunFiniArrays(loader, interpreter);
}

void PCodeInterpreterDestruct(PCodeInterpreter* interpreter) {
  free(interpreter->stack);
}
