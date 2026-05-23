//
//  aarch64_interpreter.c
//  aarch64_interpreter
//

#include "aarch64_interpreter.h"
#include "aarch64_syscalls.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t ReadX(AARCH64Interpreter* interpreter, int reg) {
  if (reg == 31) {
    return 0;
  }
  return interpreter->x[reg];
}

static void WriteX(AARCH64Interpreter* interpreter, int reg, uint64_t value) {
  if (reg == 31) {
    return;
  }
  interpreter->x[reg] = value;
}

static uint64_t ReadSp(AARCH64Interpreter* interpreter, int reg) {
  if (reg == 31) {
    return interpreter->sp;
  }
  return ReadX(interpreter, reg);
}

static bool GuestAddressOk(Loader* loader, uint64_t addr, size_t size) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return false;
}

static bool InterpreterAddressOk(AARCH64Interpreter* interpreter, uint64_t addr,
                                 size_t size) {
  if (interpreter->stack != NULL) {
    uint64_t start = (uint64_t)(uintptr_t)interpreter->stack;
    uint64_t end = start + AARCH64_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return GuestAddressOk(interpreter->loader, addr, size);
}

static void WriteSp(AARCH64Interpreter* interpreter, int reg, uint64_t value) {
  if (reg == 31) {
    interpreter->sp = value;
    return;
  }
  WriteX(interpreter, reg, value);
}

static uint32_t Fetch32(AARCH64Interpreter* interpreter) {
  if (GuestAddressOk(interpreter->loader, interpreter->pc, 4)) {
    return *(uint32_t*)(uintptr_t)interpreter->pc;
  }
  return *(uint32_t*)(uintptr_t)interpreter->pc;
}

static void Store64(AARCH64Interpreter* interpreter, uint64_t addr,
                    uint64_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Store64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint64_t*)(uintptr_t)addr = value;
}

static void Store32(AARCH64Interpreter* interpreter, uint64_t addr,
                    uint32_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint32_t*)(uintptr_t)addr = value;
}

static uint64_t Load64(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Load64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint64_t*)(uintptr_t)addr;
}

static uint32_t Load32(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint32_t*)(uintptr_t)addr;
}

static int64_t SignExtend64(uint64_t value, int bits) {
  uint64_t sign_bit = 1ULL << (bits - 1);
  return (int64_t)((value ^ sign_bit) - sign_bit);
}

static bool ExecuteMoveWide(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int opc = (insn >> 29) & 3;
  int hw = (insn >> 21) & 3;
  uint16_t imm16 = (uint16_t)((insn >> 5) & 0xffff);
  int rd = insn & 0x1f;
  if (hw > (sf ? 3 : 1)) {
    return false;
  }
  int shift = hw * 16;
  uint64_t piece = (uint64_t)imm16 << shift;
  uint64_t value = ReadX(interpreter, rd);
  switch (opc) {
    case 0:
      value = ~piece;
      break;
    case 2:
      value = piece;
      break;
    case 3:
      value = (value & ~(0xffffULL << shift)) | piece;
      break;
    default:
      return false;
  }
  if (!sf) {
    value &= 0xffffffff;
  }
  WriteX(interpreter, rd, value);
  return true;
}

static bool ExecuteLogicalShifted(AARCH64Interpreter* interpreter,
                                  uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int opc = (insn >> 29) & 3;
  int shift_type = (insn >> 22) & 3;
  int rm = (insn >> 16) & 0x1f;
  int imm6 = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  uint64_t lhs = ReadX(interpreter, rn);
  uint64_t rhs = ReadX(interpreter, rm);
  int width = sf ? 64 : 32;
  if (imm6 >= width) {
    return false;
  }
  switch (shift_type) {
    case 0:
      rhs <<= imm6;
      break;
    case 1:
      rhs = sf ? (rhs >> imm6) : ((rhs & 0xffffffff) >> imm6);
      break;
    case 2:
      rhs = (uint64_t)SignExtend64(rhs, width) >> imm6;
      break;
    case 3:
      rhs = (rhs << (width - imm6)) | (rhs >> imm6);
      break;
  }
  if (!sf) {
    lhs &= 0xffffffff;
    rhs &= 0xffffffff;
  }
  uint64_t result = 0;
  switch (opc) {
    case 0:
      result = lhs & rhs;
      break;
    case 1:
      result = lhs | rhs;
      break;
    case 2:
      result = lhs ^ rhs;
      break;
    case 3:
      result = lhs & ~rhs;
      break;
    default:
      return false;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  WriteX(interpreter, rd, result);
  return true;
}

static bool ExecuteAddSubReg(AARCH64Interpreter* interpreter, uint32_t insn) {
  if ((insn & 0x1F200000) != 0x0B000000) {
    return false;
  }
  bool sf = (insn >> 31) & 1;
  bool op = (insn >> 30) & 1;
  bool s = (insn >> 29) & 1;
  if (s) {
    return false;
  }
  int shift_type = (insn >> 22) & 3;
  int rm = (insn >> 16) & 0x1f;
  int imm6 = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  uint64_t lhs = ReadX(interpreter, rn);
  uint64_t rhs = ReadX(interpreter, rm);
  int width = sf ? 64 : 32;
  if (imm6 >= width) {
    return false;
  }
  switch (shift_type) {
    case 0:
      rhs <<= imm6;
      break;
    case 1:
      rhs = sf ? (rhs >> imm6) : ((rhs & 0xffffffff) >> imm6);
      break;
    case 2:
      rhs = (uint64_t)SignExtend64(rhs, width) >> imm6;
      break;
    case 3:
      rhs = (rhs << (width - imm6)) | (rhs >> imm6);
      break;
  }
  if (!sf) {
    lhs &= 0xffffffff;
    rhs &= 0xffffffff;
  }
  uint64_t result = op ? (lhs - rhs) : (lhs + rhs);
  if (!sf) {
    result &= 0xffffffff;
  }
  WriteSp(interpreter, rd, result);
  return true;
}

static bool ExecuteBitfieldMove(AARCH64Interpreter* interpreter, uint32_t insn) {
  if ((insn & 0x1F800000) != 0x13800000 &&
      (insn & 0x1F800000) != 0x13000000) {
    return false;
  }
  bool sf = (insn >> 31) & 1;
  int opc = (insn >> 29) & 3;
  int immr = (insn >> 16) & 0x3f;
  int imms = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  uint64_t value = ReadX(interpreter, rn);
  if (!sf) {
    value &= 0xffffffff;
  }
  int width = sf ? 64 : 32;
  uint64_t result = 0;
  if (opc == 0) {
    result = (uint64_t)SignExtend64(value << ((width - immr) & (width - 1)),
                                    (imms + 1));
  } else if (opc == 2) {
    result = value >> immr;
    if (imms + 1 < width) {
      result &= (1ULL << (imms + 1)) - 1;
    }
  } else {
    return false;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  WriteX(interpreter, rd, result);
  return true;
}

static bool ExecuteAddSubImm(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  bool op = (insn >> 30) & 1;
  bool s = (insn >> 29) & 1;
  int sh = (insn >> 22) & 1;
  uint64_t imm = (insn >> 10) & 0xfff;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  if (sh) {
    imm <<= 12;
  }
  uint64_t lhs = ReadSp(interpreter, rn);
  if (!sf) {
    lhs &= 0xffffffff;
  }
  uint64_t result = op ? (lhs - imm) : (lhs + imm);
  if (s) {
    (void)result;
    return false;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  WriteSp(interpreter, rd, result);
  return true;
}

static bool ExecuteBranchImm(AARCH64Interpreter* interpreter, uint32_t insn,
                             bool link, bool* pc_updated) {
  int64_t imm26 = SignExtend64(insn & 0x03ffffff, 26);
  int64_t offset = imm26 << 2;
  if (link) {
    WriteX(interpreter, AARCH64_LR_REG, interpreter->pc + 4);
  }
  interpreter->pc = (uint64_t)((int64_t)interpreter->pc + offset);
  *pc_updated = true;
  return true;
}

static bool ExecuteBranchReg(AARCH64Interpreter* interpreter, uint32_t insn,
                             bool* pc_updated) {
  int opc = (insn >> 21) & 0xf;
  int rn = (insn >> 5) & 0x1f;
  uint64_t target = ReadX(interpreter, rn);
  switch (opc) {
    case 0:
      interpreter->pc = target;
      *pc_updated = true;
      return true;
    case 1:
      WriteX(interpreter, AARCH64_LR_REG, interpreter->pc + 4);
      interpreter->pc = target;
      *pc_updated = true;
      return true;
    case 2:
      interpreter->pc = target;
      *pc_updated = true;
      return true;
    default:
      return false;
  }
}

static bool ExecuteCompareBranch(AARCH64Interpreter* interpreter, uint32_t insn,
                                 bool cbnz, bool* pc_updated) {
  bool sf = (insn >> 31) & 1;
  int64_t imm19 = SignExtend64((insn >> 5) & 0x7ffff, 19);
  int rt = insn & 0x1f;
  uint64_t value = ReadX(interpreter, rt);
  if (!sf) {
    value &= 0xffffffff;
  }
  bool taken = cbnz ? (value != 0) : (value == 0);
  if (taken) {
    interpreter->pc = (uint64_t)((int64_t)interpreter->pc + (imm19 << 2));
    *pc_updated = true;
  }
  return true;
}

static bool ExecuteLoadStoreImm(AARCH64Interpreter* interpreter,
                                uint32_t insn) {
  bool opc1 = (insn >> 23) & 1;
  bool vr = (insn >> 26) & 1;
  if (vr) {
    return false;
  }
  uint64_t imm12 = (insn >> 10) & 0xfff;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;
  int size = (insn >> 30) & 3;
  uint64_t addr = ReadSp(interpreter, rn) + (imm12 << size);
  if (opc1) {
    if (size == 3) {
      Store64(interpreter, addr, ReadX(interpreter, rt));
    } else if (size == 2) {
      Store32(interpreter, addr, (uint32_t)ReadX(interpreter, rt));
    } else {
      return false;
    }
  } else {
    if (size == 3) {
      WriteX(interpreter, rt, Load64(interpreter, addr));
    } else if (size == 2) {
      WriteX(interpreter, rt, Load32(interpreter, addr));
    } else {
      return false;
    }
  }
  return true;
}

static bool ExecuteLoadStorePair(AARCH64Interpreter* interpreter, uint32_t insn) {
  if ((insn & 0x3B800000) != 0x29000000) {
    return false;
  }
  bool load = ((insn >> 22) & 1) != 0;
  int opc = (insn >> 30) & 3;
  int64_t imm7 = SignExtend64((insn >> 15) & 0x7f, 7);
  int rt2 = (insn >> 10) & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;
  int64_t offset = imm7 * 8;
  uint64_t addr = ReadSp(interpreter, rn);
  if (opc == 0) {
    addr = (uint64_t)((int64_t)addr + offset);
  } else if (opc == 2) {
    addr = (uint64_t)((int64_t)addr + offset);
    if (rn == 31) {
      interpreter->sp = addr;
    }
  }
  if (load) {
    WriteX(interpreter, rt, Load64(interpreter, addr));
    WriteX(interpreter, rt2, Load64(interpreter, addr + 8));
  } else {
    Store64(interpreter, addr, ReadX(interpreter, rt));
    Store64(interpreter, addr + 8, ReadX(interpreter, rt2));
  }
  if (opc == 1 && rn == 31) {
    interpreter->sp = (uint64_t)((int64_t)addr + offset);
  }
  return true;
}

static bool ExecuteSvc(AARCH64Interpreter* interpreter, uint32_t insn) {
  (void)insn;
  int64_t result = AARCH64HandleSyscall(
      interpreter, (int64_t)ReadX(interpreter, AARCH64_SYSCALL_REG),
      (int64_t)ReadX(interpreter, 0), (int64_t)ReadX(interpreter, 1),
      (int64_t)ReadX(interpreter, 2), (int64_t)ReadX(interpreter, 3),
      (int64_t)ReadX(interpreter, 4), (int64_t)ReadX(interpreter, 5));
  WriteX(interpreter, 0, (uint64_t)result);
  return true;
}

static bool ExecuteAdrp(AARCH64Interpreter* interpreter, uint32_t insn) {
  int rd = insn & 0x1f;
  int64_t immhi = (insn >> 5) & 0x7ffff;
  int64_t immlo = (insn >> 29) & 3;
  int64_t imm = SignExtend64((immhi << 2) | immlo, 21);
  uint64_t pc_page = interpreter->pc & ~0xfffULL;
  WriteX(interpreter, rd, pc_page + ((uint64_t)imm << 12));
  return true;
}

static bool ExecuteLoadLiteral(AARCH64Interpreter* interpreter, uint32_t insn) {
  int rt = insn & 0x1f;
  int64_t imm19 = SignExtend64((insn >> 5) & 0x7ffff, 19);
  uint64_t addr = (interpreter->pc & ~3ULL) + (uint64_t)(imm19 << 2);
  WriteX(interpreter, rt, Load64(interpreter, addr));
  return true;
}

static bool ExecuteInstruction(AARCH64Interpreter* interpreter, uint32_t insn,
                               bool* pc_updated) {
  *pc_updated = false;
  if ((insn & 0xFF000000) == 0x58000000) {
    return ExecuteLoadLiteral(interpreter, insn);
  }
  if ((insn & 0x9F000000) == 0x90000000) {
    return ExecuteAdrp(interpreter, insn);
  }
  if ((insn & 0x1F000000) == 0x11000000) {
    return ExecuteAddSubImm(interpreter, insn);
  }
  if ((insn & 0x1F200000) == 0x0B000000) {
    return ExecuteAddSubReg(interpreter, insn);
  }
  if ((insn & 0x1F800000) == 0x12800000) {
    return ExecuteMoveWide(interpreter, insn);
  }
  if ((insn & 0x1F800000) == 0x13800000 ||
      (insn & 0x1F800000) == 0x13000000) {
    return ExecuteBitfieldMove(interpreter, insn);
  }
  if ((insn & 0x1F200000) == 0x0A000000) {
    return ExecuteLogicalShifted(interpreter, insn);
  }
  if ((insn & 0xFC000000) == 0x14000000) {
    return ExecuteBranchImm(interpreter, insn, false, pc_updated);
  }
  if ((insn & 0xFC000000) == 0x94000000) {
    return ExecuteBranchImm(interpreter, insn, true, pc_updated);
  }
  if ((insn & 0xFFFFFC1F) == 0xD65F0000 || (insn & 0xFFFFFC1F) == 0xD61F0000 ||
      (insn & 0xFFFFFC1F) == 0xD63F0000) {
    return ExecuteBranchReg(interpreter, insn, pc_updated);
  }
  if ((insn & 0x7F000000) == 0x34000000) {
    return ExecuteCompareBranch(interpreter, insn, false, pc_updated);
  }
  if ((insn & 0x7F000000) == 0x35000000) {
    return ExecuteCompareBranch(interpreter, insn, true, pc_updated);
  }
  if ((insn & 0x3B000000) == 0x39000000) {
    return ExecuteLoadStoreImm(interpreter, insn);
  }
  if ((insn & 0x3B800000) == 0x29000000) {
    return ExecuteLoadStorePair(interpreter, insn);
  }
  if ((insn & 0xFFE0001F) == 0xD4000001) {
    return ExecuteSvc(interpreter, insn);
  }
  if (insn == 0xD503201Fu || insn == 0xD503241Fu) {
    return true;
  }
  return false;
}

void AARCH64InterpreterDumpRegisters(AARCH64Interpreter* interpreter) {
  for (int i = 0; i < 31; i += 2) {
    printf("x%-2d 0x%016" PRIx64 "    x%-2d 0x%016" PRIx64 "\n", i,
           interpreter->x[i], i + 1, interpreter->x[i + 1]);
  }
  printf("sp 0x%016" PRIx64 "\n", interpreter->sp);
  printf("pc 0x%016" PRIx64 "\n", interpreter->pc);
}

void AARCH64InterpreterInit(AARCH64Interpreter* interpreter, Loader* loader,
                            uint64_t entry_address, int argc, char** argv,
                            bool trace_registers, bool trace_instructions) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->trace_registers = trace_registers;
  interpreter->trace_instructions = trace_instructions;
  interpreter->stack = malloc(AARCH64_STACK_SIZE);
  interpreter->sp =
      (uint64_t)(uintptr_t)(interpreter->stack + AARCH64_STACK_SIZE);
  interpreter->sp &= ~0xFULL;
  interpreter->pc = entry_address;
  interpreter->running = true;
  WriteX(interpreter, 0, (uint64_t)argc);
  if (!loader->is_static) {
    WriteX(interpreter, 1, entry_address);
  } else {
    WriteX(interpreter, 1, (uint64_t)(uintptr_t)argv);
  }
  WriteX(interpreter, AARCH64_LR_REG, 0);
}

int AARCH64InterpreterRun(AARCH64Interpreter* interpreter) {
  while (interpreter->running) {
    if (interpreter->pc == 0) {
      interpreter->exit_code = (int)ReadX(interpreter, 0);
      interpreter->running = false;
      break;
    }
    uint32_t insn = Fetch32(interpreter);
    if (interpreter->trace_instructions) {
      SymbolScope* sym =
          LoaderFindSymbolAndCacheResult(interpreter->loader, interpreter->pc);
      if (sym != NULL && sym->name != NULL) {
        printf("%s+0x%" PRIx64 ": ", sym->name,
               interpreter->pc - sym->start);
      }
      printf("0x%016" PRIx64 ": %08x\n", interpreter->pc, insn);
    }
    if (interpreter->trace_registers) {
      memcpy(interpreter->old_x, interpreter->x, sizeof(interpreter->old_x));
      interpreter->old_sp = interpreter->sp;
    }
    bool pc_updated = false;
    if (!ExecuteInstruction(interpreter, insn, &pc_updated)) {
      fprintf(stderr, "Unsupported instruction 0x%08x at 0x%" PRIx64 "\n", insn,
              interpreter->pc);
      AARCH64InterpreterDumpRegisters(interpreter);
      exit(1);
    }
    if (!pc_updated) {
      interpreter->pc += 4;
    }
    if (interpreter->trace_registers) {
      for (int i = 0; i < 31; i++) {
        if (interpreter->x[i] != interpreter->old_x[i]) {
          printf("x%d: 0x%" PRIx64 " -> 0x%" PRIx64 "\n", i,
                 interpreter->old_x[i], interpreter->x[i]);
        }
      }
      if (interpreter->sp != interpreter->old_sp) {
        printf("sp: 0x%" PRIx64 " -> 0x%" PRIx64 "\n", interpreter->old_sp,
               interpreter->sp);
      }
    }
  }
  return interpreter->exit_code;
}

void AARCH64InterpreterDestruct(AARCH64Interpreter* interpreter) {
  free(interpreter->stack);
  interpreter->stack = NULL;
}
