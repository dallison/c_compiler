//
//  x86_64_interpreter.c
//  x86_64_interpreter
//

#include "x86_64_interpreter.h"
#include "x86_64_syscalls.h"
#include "map.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  bool present;
  bool w;
  bool r;
  bool x;
  bool b;
} REX;

typedef struct {
  int mod;
  int reg;
  int rm;
  int64_t disp;
  bool has_sib;
  int scale;
  int index;
  int base;
} ModRM;

static uint64_t ReadReg(X86_64Interpreter* interpreter, int reg) {
  if (reg == X86_REG_RSP) {
    return interpreter->rsp;
  }
  if (reg == X86_REG_RBP) {
    return interpreter->rbp;
  }
  return interpreter->iregs[reg];
}

static void WriteReg(X86_64Interpreter* interpreter, int reg, uint64_t value) {
  if (reg == X86_REG_RSP) {
    interpreter->rsp = value;
    return;
  }
  if (reg == X86_REG_RBP) {
    interpreter->rbp = value;
    return;
  }
  interpreter->iregs[reg] = value;
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

static bool InterpreterAddressOk(X86_64Interpreter* interpreter, uint64_t addr,
                                 size_t size) {
  if (interpreter->stack != NULL) {
    uint64_t start = (uint64_t)(uintptr_t)interpreter->stack;
    uint64_t end = start + X86_64_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return GuestAddressOk(interpreter->loader, addr, size);
}

static void Store64(X86_64Interpreter* interpreter, uint64_t addr,
                    uint64_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Store64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint64_t*)(uintptr_t)addr = value;
}

static void Store32(X86_64Interpreter* interpreter, uint64_t addr,
                    uint32_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint32_t*)(uintptr_t)addr = value;
}

static COMPILER_UNUSED void Store16(X86_64Interpreter* interpreter, uint64_t addr,
                    uint16_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Store16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint16_t*)(uintptr_t)addr = value;
}

static void Store8(X86_64Interpreter* interpreter, uint64_t addr, uint8_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Store8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  *(uint8_t*)(uintptr_t)addr = value;
}

static uint64_t Load64(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Load64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint64_t*)(uintptr_t)addr;
}

static uint32_t Load32(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint32_t*)(uintptr_t)addr;
}

static COMPILER_UNUSED uint16_t Load16(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Load16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint16_t*)(uintptr_t)addr;
}

static uint8_t Load8(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Load8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint8_t*)(uintptr_t)addr;
}

static COMPILER_UNUSED int64_t SignExtend64(uint64_t value, int bits) {
  uint64_t sign_bit = 1ULL << (bits - 1);
  return (int64_t)((value ^ sign_bit) - sign_bit);
}

static uint8_t Fetch8At(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!GuestAddressOk(interpreter->loader, addr, 1) &&
      !InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Fetch outside mapped memory at 0x%" PRIx64 "\n", addr);
    exit(1);
  }
  return *(uint8_t*)(uintptr_t)addr;
}

static uint8_t Fetch8(X86_64Interpreter* interpreter, size_t* pos) {
  uint8_t value = Fetch8At(interpreter, interpreter->rip + *pos);
  (*pos)++;
  return value;
}

static uint32_t Fetch32(X86_64Interpreter* interpreter, size_t* pos) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    value |= (uint32_t)Fetch8(interpreter, pos) << (8 * i);
  }
  return value;
}

static uint64_t Fetch64(X86_64Interpreter* interpreter, size_t* pos) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= (uint64_t)Fetch8(interpreter, pos) << (8 * i);
  }
  return value;
}

static REX ParseRex(X86_64Interpreter* interpreter, size_t* pos, uint8_t first) {
  REX rex = {0};
  if (first < 0x40 || first > 0x4F) {
    return rex;
  }
  rex.present = true;
  rex.w = (first >> 3) & 1;
  rex.r = (first >> 2) & 1;
  rex.x = (first >> 1) & 1;
  rex.b = first & 1;
  (void)interpreter;
  (void)pos;
  return rex;
}

static bool DecodeModRM(X86_64Interpreter* interpreter, size_t* pos, REX rex,
                        bool addr_size_64, ModRM* out) {
  uint8_t modrm = Fetch8(interpreter, pos);
  out->mod = (modrm >> 6) & 3;
  out->reg = ((modrm >> 3) & 7) | (rex.r ? 8 : 0);
  out->rm = (modrm & 7) | (rex.b ? 8 : 0);
  out->disp = 0;
  out->has_sib = false;

  if (!addr_size_64) {
    return false;
  }

  if (out->mod == 3) {
    return true;
  }

  if (out->rm == 4) {
    uint8_t sib = Fetch8(interpreter, pos);
    out->has_sib = true;
    out->scale = (sib >> 6) & 3;
    out->index = ((sib >> 3) & 7) | (rex.x ? 8 : 0);
    out->base = (sib & 7) | (rex.b ? 8 : 0);
    out->rm = out->base;
  }

  if (out->mod == 0 && out->rm == 5) {
    out->disp = (int32_t)Fetch32(interpreter, pos);
    out->rm = -1;
    return true;
  }

  if (out->mod == 0) {
    out->disp = 0;
  } else if (out->mod == 1) {
    out->disp = (int8_t)Fetch8(interpreter, pos);
  } else {
    out->disp = (int32_t)Fetch32(interpreter, pos);
  }
  return true;
}

static uint64_t EffectiveAddress(X86_64Interpreter* interpreter, const ModRM* modrm,
                                 size_t insn_len) {
  if (modrm->mod == 3) {
    return ReadReg(interpreter, modrm->rm);
  }
  uint64_t addr = 0;
  if (modrm->rm >= 0) {
    addr = ReadReg(interpreter, modrm->rm);
  }
  if (modrm->has_sib) {
    if (modrm->index != 4) {
      addr += ReadReg(interpreter, modrm->index) << modrm->scale;
    }
  }
  addr = (uint64_t)((int64_t)addr + modrm->disp);
  if (modrm->mod == 0 && modrm->rm < 0) {
    addr = interpreter->rip + insn_len + modrm->disp;
  }
  return addr;
}

static void Push64(X86_64Interpreter* interpreter, uint64_t value) {
  interpreter->rsp -= 8;
  Store64(interpreter, interpreter->rsp, value);
}

static uint64_t Pop64(X86_64Interpreter* interpreter) {
  uint64_t value = Load64(interpreter, interpreter->rsp);
  interpreter->rsp += 8;
  return value;
}

static bool ExecuteMovImm(X86_64Interpreter* interpreter, size_t* pos, REX rex,
                          uint8_t opcode) {
  int reg = (opcode & 0x7) | (rex.b ? 8 : 0);
  if (rex.w) {
    WriteReg(interpreter, reg, Fetch64(interpreter, pos));
  } else {
    WriteReg(interpreter, reg, (uint64_t)(int64_t)(int32_t)Fetch32(interpreter, pos));
  }
  return true;
}

static void UpdateFlags(X86_64Interpreter* interpreter, uint64_t result,
                        bool width64) {
  uint64_t mask = width64 ? UINT64_MAX : 0xffffffffu;
  result &= mask;
  interpreter->zf = result == 0;
  interpreter->sf =
      width64 ? ((result >> 63) & 1) != 0 : ((result >> 31) & 1) != 0;
}

static bool ExecuteMovRegMem(X86_64Interpreter* interpreter, size_t insn_len,
                             REX rex, uint8_t opcode, ModRM modrm) {
  bool byte_op = (opcode == 0x88 || opcode == 0x8A);
  bool store = (opcode == 0x88 || opcode == 0x89);
  if (modrm.mod == 3) {
    if (store) {
      if (byte_op) {
        WriteReg(interpreter, modrm.rm,
                 (ReadReg(interpreter, modrm.rm) & ~0xffULL) |
                     (ReadReg(interpreter, modrm.reg) & 0xff));
      } else if (rex.w) {
        WriteReg(interpreter, modrm.rm, ReadReg(interpreter, modrm.reg));
      } else {
        WriteReg(interpreter, modrm.rm,
                 (ReadReg(interpreter, modrm.rm) & ~0xffffffffULL) |
                     (ReadReg(interpreter, modrm.reg) & 0xffffffff));
      }
    } else {
      if (byte_op) {
        WriteReg(interpreter, modrm.reg, ReadReg(interpreter, modrm.rm) & 0xff);
      } else if (rex.w) {
        WriteReg(interpreter, modrm.reg, ReadReg(interpreter, modrm.rm));
      } else {
        WriteReg(interpreter, modrm.reg,
                 ReadReg(interpreter, modrm.rm) & 0xffffffff);
      }
    }
    return true;
  }

  uint64_t addr = EffectiveAddress(interpreter, &modrm, insn_len);
  if (store) {
    if (byte_op) {
      Store8(interpreter, addr, (uint8_t)ReadReg(interpreter, modrm.reg));
    } else if (rex.w) {
      Store64(interpreter, addr, ReadReg(interpreter, modrm.reg));
    } else {
      Store32(interpreter, addr, (uint32_t)ReadReg(interpreter, modrm.reg));
    }
  } else {
    if (byte_op) {
      WriteReg(interpreter, modrm.reg, Load8(interpreter, addr));
    } else if (rex.w) {
      WriteReg(interpreter, modrm.reg, Load64(interpreter, addr));
    } else {
      WriteReg(interpreter, modrm.reg, (uint64_t)Load32(interpreter, addr));
    }
  }
  return true;
}

static bool ExecuteLea(X86_64Interpreter* interpreter, size_t insn_len,
                         ModRM modrm) {
  if (modrm.mod == 3) {
    return false;
  }
  WriteReg(interpreter, modrm.reg,
           EffectiveAddress(interpreter, &modrm, insn_len));
  return true;
}

static bool ExecuteAluRegMem(X86_64Interpreter* interpreter, size_t insn_len,
                             REX rex, uint8_t opcode, ModRM modrm,
                             bool reg_is_dest) {
  bool byte_op = (opcode & 1) != 0;
  int op = (opcode >> 3) & 7;
  uint64_t lhs = 0;
  uint64_t rhs = 0;
  int dst = reg_is_dest ? modrm.reg : modrm.rm;
  int src = reg_is_dest ? modrm.rm : modrm.reg;
  uint64_t mem_addr = 0;

  if (modrm.mod == 3) {
    lhs = ReadReg(interpreter, dst);
    rhs = ReadReg(interpreter, src);
  } else if (reg_is_dest) {
    lhs = ReadReg(interpreter, modrm.reg);
    mem_addr = EffectiveAddress(interpreter, &modrm, insn_len);
    if (byte_op) {
      rhs = Load8(interpreter, mem_addr);
    } else if (rex.w) {
      rhs = Load64(interpreter, mem_addr);
    } else {
      rhs = Load32(interpreter, mem_addr);
    }
  } else {
    mem_addr = EffectiveAddress(interpreter, &modrm, insn_len);
    if (byte_op) {
      lhs = Load8(interpreter, mem_addr);
    } else if (rex.w) {
      lhs = Load64(interpreter, mem_addr);
    } else {
      lhs = Load32(interpreter, mem_addr);
    }
    rhs = ReadReg(interpreter, modrm.reg);
  }

  uint64_t result = 0;
  switch (op) {
    case 0:
      result = lhs + rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 1:
      result = lhs | rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 2:
      result = lhs + rhs + (interpreter->cf ? 1 : 0);
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 3:
      result = lhs - rhs - (interpreter->cf ? 1 : 0);
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 4:
      result = lhs & rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 5:
      result = lhs - rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 6:
      result = lhs ^ rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 7:
      result = lhs - rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      return true;
    default:
      return false;
  }

  if (!rex.w && !byte_op) {
    result = (uint64_t)(uint32_t)result;
  } else if (byte_op) {
    result = (uint64_t)(uint8_t)result;
  }

  if (modrm.mod == 3) {
    WriteReg(interpreter, dst, result);
  } else if (!reg_is_dest) {
    if (byte_op) {
      Store8(interpreter, mem_addr, (uint8_t)result);
    } else if (rex.w) {
      Store64(interpreter, mem_addr, result);
    } else {
      Store32(interpreter, mem_addr, (uint32_t)result);
    }
  }
  return true;
}

static bool ExecuteAluImm(X86_64Interpreter* interpreter, REX rex, uint8_t opcode,
                          ModRM modrm, int64_t imm) {
  int op = modrm.reg;
  bool byte_op = (opcode == 0x80);
  if (modrm.mod != 3) {
    return false;
  }
  uint64_t lhs = ReadReg(interpreter, modrm.rm);
  uint64_t rhs = (uint64_t)imm;
  uint64_t result = 0;
  switch (op) {
    case 0:
      result = lhs + rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 1:
      result = lhs | rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 2:
      result = lhs + rhs + (interpreter->cf ? 1 : 0);
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 3:
      result = lhs - rhs - (interpreter->cf ? 1 : 0);
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 4:
      result = lhs & rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 5:
      result = lhs - rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 6:
      result = lhs ^ rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      break;
    case 7:
      result = lhs - rhs;
      UpdateFlags(interpreter, result, rex.w && !byte_op);
      return true;
    default:
      return false;
  }
  if (!rex.w && !byte_op) {
    result = (uint64_t)(uint32_t)result;
  } else if (byte_op) {
    result = (uint64_t)(uint8_t)result;
  }
  WriteReg(interpreter, modrm.rm, result);
  return true;
}

static bool ExecuteTest(X86_64Interpreter* interpreter, size_t insn_len, REX rex,
                        uint8_t opcode, ModRM modrm) {
  bool byte_op = (opcode & 1) != 0;
  uint64_t lhs = 0;
  uint64_t rhs = 0;
  if (modrm.mod == 3) {
    lhs = ReadReg(interpreter, modrm.rm);
    rhs = ReadReg(interpreter, modrm.reg);
  } else {
    uint64_t mem_addr = EffectiveAddress(interpreter, &modrm, insn_len);
    if (byte_op) {
      lhs = Load8(interpreter, mem_addr);
    } else if (rex.w) {
      lhs = Load64(interpreter, mem_addr);
    } else {
      lhs = Load32(interpreter, mem_addr);
    }
    rhs = ReadReg(interpreter, modrm.reg);
  }
  if (byte_op) {
    lhs &= 0xff;
    rhs &= 0xff;
  } else if (!rex.w) {
    lhs &= 0xffffffff;
    rhs &= 0xffffffff;
  }
  UpdateFlags(interpreter, lhs & rhs, !byte_op && rex.w);
  return true;
}

static bool ExecuteSetcc(X86_64Interpreter* interpreter, uint8_t cc,
                         ModRM modrm) {
  if (modrm.mod != 3) {
    return false;
  }
  bool set = false;
  switch (cc) {
    case 0x4:
      set = interpreter->zf;
      break;
    case 0x5:
      set = !interpreter->zf;
      break;
    case 0x8:
      set = interpreter->sf;
      break;
    case 0x9:
      set = !interpreter->sf;
      break;
    case 0xC:
      set = interpreter->sf != interpreter->of;
      break;
    case 0xD:
      set = interpreter->sf == interpreter->of;
      break;
    case 0xE:
      set = interpreter->cf || interpreter->zf;
      break;
    case 0xF:
      set = !interpreter->cf && !interpreter->zf;
      break;
    default:
      return false;
  }
  uint64_t reg = ReadReg(interpreter, modrm.rm);
  reg = (reg & ~0xffULL) | (set ? 1ULL : 0ULL);
  WriteReg(interpreter, modrm.rm, reg);
  return true;
}

static bool ExecuteSyscall(X86_64Interpreter* interpreter) {
  int64_t result = X86_64HandleSyscall(
      interpreter, (int64_t)ReadReg(interpreter, X86_REG_RAX),
      (int64_t)ReadReg(interpreter, X86_REG_RDI),
      (int64_t)ReadReg(interpreter, X86_REG_RSI),
      (int64_t)ReadReg(interpreter, X86_REG_RDX),
      (int64_t)ReadReg(interpreter, X86_REG_RCX),
      (int64_t)ReadReg(interpreter, X86_REG_R8),
      (int64_t)ReadReg(interpreter, X86_REG_R9));
  WriteReg(interpreter, X86_REG_RAX, (uint64_t)result);
  return true;
}

static bool ExecuteInstruction(X86_64Interpreter* interpreter, size_t* insn_len,
                               bool* rip_updated) {
  *rip_updated = false;
  interpreter->rip_updated = false;
  size_t pos = 0;
  uint8_t b0 = Fetch8(interpreter, &pos);

  REX rex = {0};
  if (b0 >= 0x40 && b0 <= 0x4F) {
    rex = ParseRex(interpreter, &pos, b0);
    b0 = Fetch8(interpreter, &pos);
  }

  if (b0 == 0x0F) {
    uint8_t b1 = Fetch8(interpreter, &pos);
    if (b1 == 0x05) {
      *insn_len = pos;
      if (!ExecuteSyscall(interpreter)) {
        return false;
      }
      *rip_updated = interpreter->rip_updated;
      return true;
    }
    if (b1 >= 0x80 && b1 <= 0x8F) {
      uint8_t cc = b1 & 0x0F;
      int32_t disp = (int32_t)Fetch32(interpreter, &pos);
      bool take = false;
      switch (cc) {
        case 0x4:
          take = interpreter->zf;
          break;
        case 0x5:
          take = !interpreter->zf;
          break;
        case 0x8:
          take = interpreter->sf;
          break;
        case 0x9:
          take = !interpreter->sf;
          break;
        case 0xC:
          take = interpreter->sf != interpreter->of;
          break;
        case 0xD:
          take = interpreter->sf == interpreter->of;
          break;
        case 0xE:
          take = interpreter->cf || interpreter->zf;
          break;
        case 0xF:
          take = !interpreter->cf && !interpreter->zf;
          break;
        default:
          return false;
      }
      if (take) {
        interpreter->rip += pos + (uint64_t)(int64_t)disp;
        *rip_updated = true;
      }
      *insn_len = pos;
      return true;
    }
    if (b1 >= 0x90 && b1 <= 0x9F) {
      ModRM modrm;
      if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
        return false;
      }
      *insn_len = pos;
      return ExecuteSetcc(interpreter, b1 & 0x0F, modrm);
    }
    return false;
  }

  if (b0 >= 0xB8 && b0 <= 0xBF) {
    if (!ExecuteMovImm(interpreter, &pos, rex, b0)) {
      return false;
    }
    *insn_len = pos;
    return true;
  }

  if (b0 >= 0x50 && b0 <= 0x57) {
    int reg = (b0 - 0x50) | (rex.b ? 8 : 0);
    Push64(interpreter, ReadReg(interpreter, reg));
    *insn_len = pos;
    return true;
  }

  if (b0 >= 0x58 && b0 <= 0x5F) {
    int reg = (b0 - 0x58) | (rex.b ? 8 : 0);
    WriteReg(interpreter, reg, Pop64(interpreter));
    *insn_len = pos;
    return true;
  }

  if (b0 == 0xE8) {
    int32_t disp = (int32_t)Fetch32(interpreter, &pos);
    Push64(interpreter, interpreter->rip + pos);
    interpreter->rip = (uint64_t)((int64_t)interpreter->rip + pos + disp);
    *rip_updated = true;
    *insn_len = pos;
    return true;
  }

  if (b0 == 0xE9) {
    int32_t disp = (int32_t)Fetch32(interpreter, &pos);
    interpreter->rip = (uint64_t)((int64_t)interpreter->rip + pos + disp);
    *rip_updated = true;
    *insn_len = pos;
    return true;
  }

  if (b0 == 0xEB) {
    int8_t disp = (int8_t)Fetch8(interpreter, &pos);
    interpreter->rip = (uint64_t)((int64_t)interpreter->rip + pos + disp);
    *rip_updated = true;
    *insn_len = pos;
    return true;
  }

  if (b0 == 0xC3) {
    interpreter->rip = Pop64(interpreter);
    *rip_updated = true;
    *insn_len = pos;
    return true;
  }

  if (b0 == 0xC9) {
    interpreter->rsp = interpreter->rbp;
    interpreter->rbp = Pop64(interpreter);
    *insn_len = pos;
    return true;
  }

  if (b0 == 0x90) {
    *insn_len = pos;
    return true;
  }

  ModRM modrm;
  if (b0 == 0xC7) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    if (modrm.mod != 3 || modrm.reg != 0) {
      return false;
    }
    int64_t imm = (int32_t)Fetch32(interpreter, &pos);
    WriteReg(interpreter, modrm.rm, rex.w ? (uint64_t)imm : (uint64_t)(uint32_t)imm);
    *insn_len = pos;
    return true;
  }

  if ((b0 & 0xFE) == 0x88 || (b0 & 0xFE) == 0x8A) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteMovRegMem(interpreter, *insn_len, rex, b0, modrm);
  }

  if (b0 == 0x8D) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteLea(interpreter, *insn_len, modrm);
  }

  if (b0 == 0x84 || b0 == 0x85) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteTest(interpreter, *insn_len, rex, b0, modrm);
  }

  if (b0 == 0x01 || b0 == 0x29) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteAluRegMem(interpreter, *insn_len, rex, b0, modrm, false);
  }

  if (b0 == 0x03 || b0 == 0x2B) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteAluRegMem(interpreter, *insn_len, rex, b0, modrm, true);
  }

  if (b0 == 0x39 || b0 == 0x3B) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteAluRegMem(interpreter, *insn_len, rex, b0, modrm, b0 == 0x3B);
  }

  if (b0 == 0x81 || b0 == 0x83) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    int64_t imm = (b0 == 0x81) ? (int32_t)Fetch32(interpreter, &pos)
                               : (int8_t)Fetch8(interpreter, &pos);
    *insn_len = pos;
    return ExecuteAluImm(interpreter, rex, b0, modrm, imm);
  }

  if (b0 == 0xF7) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    if (modrm.mod != 3) {
      return false;
    }
    int op = modrm.reg & 7;
    uint64_t value = ReadReg(interpreter, modrm.rm);
    if (op == 3) {
      uint64_t result = rex.w ? (uint64_t)(-(int64_t)value)
                                : (uint64_t)(-(int32_t)(uint32_t)value);
      UpdateFlags(interpreter, result, rex.w);
      WriteReg(interpreter, modrm.rm, result);
      return true;
    }
    return false;
  }

  if (b0 == 0xFF) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    int op = modrm.reg & 7;
    if (op == 2 || op == 4) {
      uint64_t target = 0;
      if (modrm.mod == 3) {
        target = ReadReg(interpreter, modrm.rm);
      } else {
        target = Load64(interpreter, EffectiveAddress(interpreter, &modrm, pos));
      }
      if (op == 2) {
        Push64(interpreter, interpreter->rip + pos);
      }
      interpreter->rip = target;
      *rip_updated = true;
      *insn_len = pos;
      return true;
    }
    return false;
  }

  return false;
}

void X86_64InterpreterDumpRegisters(X86_64Interpreter* interpreter) {
  static const char* names[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp",
                                "rsi", "rdi", "r8",  "r9",  "r10", "r11",
                                "r12", "r13", "r14", "r15"};
  for (int i = 0; i < X86_NUM_INT_REGS; i += 2) {
    printf("%-3s 0x%016" PRIx64 "    %-3s 0x%016" PRIx64 "\n", names[i],
           ReadReg(interpreter, i), names[i + 1], ReadReg(interpreter, i + 1));
  }
  printf("rip 0x%016" PRIx64 "\n", interpreter->rip);
}

void X86_64InterpreterInit(X86_64Interpreter* interpreter, Loader* loader,
                             uint64_t entry_address, int argc, char** argv,
                             bool trace_registers, bool trace_instructions) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->trace_registers = trace_registers;
  interpreter->trace_instructions = trace_instructions;
  interpreter->stack = malloc(X86_64_STACK_SIZE);
  interpreter->rsp =
      (uint64_t)(uintptr_t)(interpreter->stack + X86_64_STACK_SIZE);
  interpreter->rsp &= ~0xFULL;
  Push64(interpreter, 0);
  interpreter->rip = entry_address;
  interpreter->running = true;

  WriteReg(interpreter, X86_REG_RDI, (uint64_t)argc);
  if (!loader->is_static) {
    WriteReg(interpreter, X86_REG_RSI, entry_address);
  } else {
    WriteReg(interpreter, X86_REG_RSI, (uint64_t)(uintptr_t)argv);
  }
}

int X86_64InterpreterRun(X86_64Interpreter* interpreter) {
  while (interpreter->running) {
    if (interpreter->rip == 0) {
      interpreter->exit_code = (int)ReadReg(interpreter, X86_REG_RAX);
      interpreter->running = false;
      break;
    }

    if (interpreter->trace_instructions) {
      SymbolScope* sym =
          LoaderFindSymbolAndCacheResult(interpreter->loader, interpreter->rip);
      if (sym != NULL && sym->name != NULL) {
        printf("%s+0x%" PRIx64 ": ", sym->name, interpreter->rip - sym->start);
      }
      printf("0x%016" PRIx64 ":", interpreter->rip);
      for (size_t i = 0; i < 8; i++) {
        if (GuestAddressOk(interpreter->loader, interpreter->rip + i, 1) ||
            InterpreterAddressOk(interpreter, interpreter->rip + i, 1)) {
          printf(" %02x", Fetch8At(interpreter, interpreter->rip + i));
        }
      }
      printf("\n");
    }

    if (interpreter->trace_registers) {
      for (int i = 0; i < X86_NUM_INT_REGS; i++) {
        interpreter->old_iregs[i] = interpreter->iregs[i];
      }
      interpreter->old_rsp = interpreter->rsp;
      interpreter->old_rbp = interpreter->rbp;
    }

    size_t insn_len = 0;
    bool rip_updated = false;
    if (!ExecuteInstruction(interpreter, &insn_len, &rip_updated)) {
      fprintf(stderr, "Unsupported instruction at 0x%" PRIx64 "\n",
              interpreter->rip);
      X86_64InterpreterDumpRegisters(interpreter);
      exit(1);
    }
    if (!rip_updated) {
      interpreter->rip += insn_len;
    }

    if (interpreter->trace_registers) {
      static const char* names[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp",
                                    "rsi", "rdi", "r8",  "r9",  "r10", "r11",
                                    "r12", "r13", "r14", "r15"};
      for (int i = 0; i < X86_NUM_INT_REGS; i++) {
        if (interpreter->iregs[i] != interpreter->old_iregs[i]) {
          printf("%s: 0x%" PRIx64 " -> 0x%" PRIx64 "\n", names[i],
                 interpreter->old_iregs[i], interpreter->iregs[i]);
        }
      }
      if (interpreter->rsp != interpreter->old_rsp) {
        printf("rsp: 0x%" PRIx64 " -> 0x%" PRIx64 "\n", interpreter->old_rsp,
               interpreter->rsp);
      }
      if (interpreter->rbp != interpreter->old_rbp) {
        printf("rbp: 0x%" PRIx64 " -> 0x%" PRIx64 "\n", interpreter->old_rbp,
               interpreter->rbp);
      }
    }
  }
  return interpreter->exit_code;
}

void X86_64InterpreterDestruct(X86_64Interpreter* interpreter) {
  free(interpreter->stack);
  interpreter->stack = NULL;
}
