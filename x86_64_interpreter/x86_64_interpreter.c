//
//  x86_64_interpreter.c
//  x86_64_interpreter
//

#include "x86_64_interpreter.h"
#include "x86_64_process.h"
#include "x86_64_syscalls.h"
#include "map.h"
#include <inttypes.h>
#include <math.h>
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
  bool rip_relative;
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

void X86_64InterpreterWriteReg(X86_64Interpreter* interpreter, int reg,
                               uint64_t value) {
  WriteReg(interpreter, reg, value);
}

SymbolScope* X86_64InterpreterFindSymbol(X86_64Interpreter* interpreter,
                                         uint64_t address) {
  if (address >= interpreter->symbol_cache.start &&
      address < interpreter->symbol_cache.end) {
    return &interpreter->symbol_cache;
  }
  bool found =
      LoaderFindSymbol(interpreter->loader, address, &interpreter->symbol_cache);
  if (!found) {
    return NULL;
  }
  return &interpreter->symbol_cache;
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

static bool ProcessMemoryOk(X86_64Interpreter* interpreter, uint64_t addr,
                            size_t size) {
  if (interpreter->process == NULL) {
    return false;
  }
  return X86_64ProcessGuestMemoryOk(interpreter->process, addr, size);
}

static bool InterpreterAddressOk(X86_64Interpreter* interpreter, uint64_t addr,
                                 size_t size) {
  if (interpreter->fs_base != 0 && interpreter->tls_block_size > 0) {
    uint64_t tls_start = interpreter->fs_base;
    uint64_t tls_end = tls_start + (uint64_t)interpreter->tls_block_size;
    if (addr >= tls_start && addr + size <= tls_end) {
      return true;
    }
  }
  if (interpreter->stack != NULL) {
    uint64_t start = (uint64_t)(uintptr_t)interpreter->stack;
    uint64_t end = start + X86_64_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  if (ProcessMemoryOk(interpreter, addr, size)) {
    return true;
  }
  return GuestAddressOk(interpreter->loader, addr, size);
}

static void Store64(X86_64Interpreter* interpreter, uint64_t addr,
                    uint64_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Store64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return;
  }
  *(uint64_t*)(uintptr_t)addr = value;
}

static void Store32(X86_64Interpreter* interpreter, uint64_t addr,
                    uint32_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return;
  }
  *(uint32_t*)(uintptr_t)addr = value;
}

static COMPILER_UNUSED void Store16(X86_64Interpreter* interpreter, uint64_t addr,
                    uint16_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Store16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return;
  }
  *(uint16_t*)(uintptr_t)addr = value;
}

static void Store8(X86_64Interpreter* interpreter, uint64_t addr, uint8_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Store8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return;
  }
  *(uint8_t*)(uintptr_t)addr = value;
}

static uint64_t Load64(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Load64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return 0;
  }
  return *(uint64_t*)(uintptr_t)addr;
}

static uint32_t Load32(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return 0;
  }
  return *(uint32_t*)(uintptr_t)addr;
}

static COMPILER_UNUSED uint16_t Load16(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Load16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return 0;
  }
  return *(uint16_t*)(uintptr_t)addr;
}

static uint8_t Load8(X86_64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Load8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    X86_64InterpreterFail(interpreter, 1);
    return 0;
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
    X86_64InterpreterFail(interpreter, 1);
    return 0;
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
  // The "SIB follows" (rm==100b) and "RIP/disp32" (rm==101b) encodings are
  // determined by the raw 3-bit rm field, *before* REX.B is folded in.  E.g.
  // r12 (REX.B + 100b) still uses a SIB byte and r13 (REX.B + 101b) is a normal
  // base, not RIP-relative.
  int raw_rm = modrm & 7;
  out->rm = raw_rm | (rex.b ? 8 : 0);
  out->disp = 0;
  out->has_sib = false;
  out->rip_relative = false;

  if (!addr_size_64) {
    return false;
  }

  if (out->mod == 3) {
    return true;
  }

  if (raw_rm == 4) {
    uint8_t sib = Fetch8(interpreter, pos);
    out->has_sib = true;
    out->scale = (sib >> 6) & 3;
    out->index = ((sib >> 3) & 7) | (rex.x ? 8 : 0);
    out->base = (sib & 7) | (rex.b ? 8 : 0);
    if (out->mod == 0 && (sib & 7) == 5) {
      out->disp = (int32_t)Fetch32(interpreter, pos);
      out->rm = -1;
      return true;
    }
    out->rm = out->base;
  } else if (out->mod == 0 && raw_rm == 5) {
    out->disp = (int32_t)Fetch32(interpreter, pos);
    out->rm = -1;
    out->rip_relative = true;
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
  if (modrm->rip_relative) {
    addr = interpreter->rip + insn_len + modrm->disp;
  } else {
    if (modrm->rm >= 0) {
      addr = ReadReg(interpreter, modrm->rm);
    }
    if (modrm->has_sib) {
      if (modrm->index != 4) {
        addr += ReadReg(interpreter, modrm->index) << modrm->scale;
      }
    }
    addr = (uint64_t)((int64_t)addr + modrm->disp);
  }
  if (interpreter->current_seg_prefix == 0x64 && interpreter->fs_base != 0) {
    addr = interpreter->fs_base + addr;
  }
  return addr;
}

// --- SSE / scalar floating-point helpers -----------------------------------

static double XmmReadDouble(X86_64Interpreter* interpreter, int reg) {
  double d;
  uint64_t bits = interpreter->xmm[reg][0];
  memcpy(&d, &bits, sizeof(d));
  return d;
}

static float XmmReadFloat(X86_64Interpreter* interpreter, int reg) {
  float f;
  uint32_t bits = (uint32_t)interpreter->xmm[reg][0];
  memcpy(&f, &bits, sizeof(f));
  return f;
}

static void XmmWriteDouble(X86_64Interpreter* interpreter, int reg, double d) {
  uint64_t bits;
  memcpy(&bits, &d, sizeof(bits));
  interpreter->xmm[reg][0] = bits;
}

// Writing a scalar single leaves the upper bits of the low lane unchanged, which
// matches the behaviour of the scalar SSE instructions on hardware (and is all
// the generated code relies on).
static void XmmWriteFloat(X86_64Interpreter* interpreter, int reg, float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(bits));
  interpreter->xmm[reg][0] =
      (interpreter->xmm[reg][0] & ~0xffffffffULL) | bits;
}

static double BitsToDouble(uint64_t bits) {
  double d;
  memcpy(&d, &bits, sizeof(d));
  return d;
}

static float BitsToFloat(uint32_t bits) {
  float f;
  memcpy(&f, &bits, sizeof(f));
  return f;
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

static void UpdateLogicalFlags(X86_64Interpreter* interpreter, uint64_t result,
                               bool width64) {
  UpdateFlags(interpreter, result, width64);
  interpreter->cf = false;
  interpreter->of = false;
}

static void UpdateAddFlags(X86_64Interpreter* interpreter, uint64_t lhs,
                           uint64_t rhs, uint64_t result, bool width64) {
  uint64_t mask = width64 ? UINT64_MAX : 0xffffffffu;
  uint64_t sign = width64 ? (1ULL << 63) : (1ULL << 31);
  lhs &= mask;
  rhs &= mask;
  result &= mask;
  UpdateFlags(interpreter, result, width64);
  interpreter->cf = result < lhs;
  interpreter->of = ((~(lhs ^ rhs) & (lhs ^ result) & sign) != 0);
}

static void UpdateSubFlags(X86_64Interpreter* interpreter, uint64_t lhs,
                           uint64_t rhs, uint64_t result, bool width64) {
  uint64_t mask = width64 ? UINT64_MAX : 0xffffffffu;
  uint64_t sign = width64 ? (1ULL << 63) : (1ULL << 31);
  lhs &= mask;
  rhs &= mask;
  result &= mask;
  UpdateFlags(interpreter, result, width64);
  interpreter->cf = lhs < rhs;
  interpreter->of = (((lhs ^ rhs) & (lhs ^ result) & sign) != 0);
}

static bool ExecuteMovRegMem(X86_64Interpreter* interpreter, size_t insn_len,
                             REX rex, uint8_t opcode, ModRM modrm, bool op16) {
  bool byte_op = (opcode == 0x88 || opcode == 0x8A);
  bool store = (opcode == 0x88 || opcode == 0x89);
  bool word_op = op16 && !byte_op && !rex.w;
  if (modrm.mod == 3) {
    if (store) {
      if (byte_op) {
        WriteReg(interpreter, modrm.rm,
                 (ReadReg(interpreter, modrm.rm) & ~0xffULL) |
                     (ReadReg(interpreter, modrm.reg) & 0xff));
      } else if (word_op) {
        WriteReg(interpreter, modrm.rm,
                 (ReadReg(interpreter, modrm.rm) & ~0xffffULL) |
                     (ReadReg(interpreter, modrm.reg) & 0xffff));
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
      } else if (word_op) {
        WriteReg(interpreter, modrm.reg,
                 (ReadReg(interpreter, modrm.reg) & ~0xffffULL) |
                     (ReadReg(interpreter, modrm.rm) & 0xffff));
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
    } else if (word_op) {
      Store16(interpreter, addr, (uint16_t)ReadReg(interpreter, modrm.reg));
    } else if (rex.w) {
      Store64(interpreter, addr, ReadReg(interpreter, modrm.reg));
    } else {
      Store32(interpreter, addr, (uint32_t)ReadReg(interpreter, modrm.reg));
    }
  } else {
    if (byte_op) {
      WriteReg(interpreter, modrm.reg, Load8(interpreter, addr));
    } else if (word_op) {
      WriteReg(interpreter, modrm.reg,
               (ReadReg(interpreter, modrm.reg) & ~0xffffULL) |
                   Load16(interpreter, addr));
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
  // For the ALU opcode families (ADD 0x00/0x01, SUB 0x28/0x29, ...), bit 0
  // selects the operand size: 0 => 8-bit byte form, 1 => 16/32/64-bit form.
  bool byte_op = (opcode & 1) == 0;
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
  bool width64 = rex.w && !byte_op;
  switch (op) {
    case 0:
      result = lhs + rhs;
      UpdateAddFlags(interpreter, lhs, rhs, result, width64);
      break;
    case 1:
      result = lhs | rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 2: {
      uint64_t carry = interpreter->cf ? 1 : 0;
      result = lhs + rhs + carry;
      UpdateAddFlags(interpreter, lhs, rhs + carry, result, width64);
      break;
    }
    case 3: {
      uint64_t carry = interpreter->cf ? 1 : 0;
      result = lhs - rhs - carry;
      UpdateSubFlags(interpreter, lhs, rhs + carry, result, width64);
      break;
    }
    case 4:
      result = lhs & rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 5:
      result = lhs - rhs;
      UpdateSubFlags(interpreter, lhs, rhs, result, width64);
      break;
    case 6:
      result = lhs ^ rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 7:
      result = lhs - rhs;
      UpdateSubFlags(interpreter, lhs, rhs, result, width64);
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
  bool width64 = rex.w && !byte_op;
  switch (op) {
    case 0:
      result = lhs + rhs;
      UpdateAddFlags(interpreter, lhs, rhs, result, width64);
      break;
    case 1:
      result = lhs | rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 2: {
      uint64_t carry = interpreter->cf ? 1 : 0;
      result = lhs + rhs + carry;
      UpdateAddFlags(interpreter, lhs, rhs + carry, result, width64);
      break;
    }
    case 3: {
      uint64_t carry = interpreter->cf ? 1 : 0;
      result = lhs - rhs - carry;
      UpdateSubFlags(interpreter, lhs, rhs + carry, result, width64);
      break;
    }
    case 4:
      result = lhs & rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 5:
      result = lhs - rhs;
      UpdateSubFlags(interpreter, lhs, rhs, result, width64);
      break;
    case 6:
      result = lhs ^ rhs;
      UpdateLogicalFlags(interpreter, result, width64);
      break;
    case 7:
      result = lhs - rhs;
      UpdateSubFlags(interpreter, lhs, rhs, result, width64);
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

static bool ExecuteShift(X86_64Interpreter* interpreter, size_t insn_len,
                         REX rex, uint8_t opcode, ModRM modrm, unsigned count) {
  bool byte_op = (opcode == 0xC0 || opcode == 0xD0 || opcode == 0xD2);
  int op = modrm.reg & 7;
  int bits = byte_op ? 8 : (rex.w ? 64 : 32);
  count &= (rex.w ? 63u : 31u);

  uint64_t value = 0;
  uint64_t mem_addr = 0;
  if (modrm.mod == 3) {
    value = ReadReg(interpreter, modrm.rm);
  } else {
    mem_addr = EffectiveAddress(interpreter, &modrm, insn_len);
    if (byte_op) {
      value = Load8(interpreter, mem_addr);
    } else if (rex.w) {
      value = Load64(interpreter, mem_addr);
    } else {
      value = Load32(interpreter, mem_addr);
    }
  }

  uint64_t masked = (bits == 64) ? value : (value & ((1ULL << bits) - 1));
  uint64_t result = value;
  switch (op) {
    case 4:  // SHL / SAL
    case 6:
      result = value << count;
      break;
    case 5:  // SHR (logical)
      result = masked >> count;
      break;
    case 7: {  // SAR (arithmetic)
      int64_t sval;
      if (byte_op) {
        sval = (int8_t)(uint8_t)masked;
      } else if (bits == 32) {
        sval = (int32_t)(uint32_t)masked;
      } else {
        sval = (int64_t)masked;
      }
      result = (uint64_t)(sval >> count);
      break;
    }
    case 0: {  // ROL
      if (count == 0) {
        result = masked;
      } else {
        result = (masked << count) | (masked >> (bits - count));
      }
      break;
    }
    case 1: {  // ROR
      if (count == 0) {
        result = masked;
      } else {
        result = (masked >> count) | (masked << (bits - count));
      }
      break;
    }
    default:
      return false;
  }

  if (byte_op) {
    result = (uint64_t)(uint8_t)result;
  } else if (!rex.w) {
    result = (uint64_t)(uint32_t)result;
  }

  if (count != 0) {
    UpdateFlags(interpreter, result, rex.w && !byte_op);
  }

  if (modrm.mod == 3) {
    WriteReg(interpreter, modrm.rm, result);
  } else if (byte_op) {
    Store8(interpreter, mem_addr, (uint8_t)result);
  } else if (rex.w) {
    Store64(interpreter, mem_addr, result);
  } else {
    Store32(interpreter, mem_addr, (uint32_t)result);
  }
  return true;
}

static bool ExecuteTest(X86_64Interpreter* interpreter, size_t insn_len, REX rex,
                        uint8_t opcode, ModRM modrm) {
  // TEST 0x84 is the 8-bit byte form; 0x85 is the 16/32/64-bit form.
  bool byte_op = (opcode & 1) == 0;
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
  UpdateLogicalFlags(interpreter, lhs & rhs, !byte_op && rex.w);
  return true;
}

static bool ExecuteSetcc(X86_64Interpreter* interpreter, uint8_t cc,
                         ModRM modrm) {
  if (modrm.mod != 3) {
    return false;
  }
  bool set = false;
  switch (cc) {
    case 0x2:
      set = interpreter->cf;
      break;
    case 0x3:
      set = !interpreter->cf;
      break;
    case 0x4:
      set = interpreter->zf;
      break;
    case 0x5:
      set = !interpreter->zf;
      break;
    case 0x6:
      set = interpreter->cf || interpreter->zf;
      break;
    case 0x7:
      set = !interpreter->cf && !interpreter->zf;
      break;
    case 0x8:
      set = interpreter->sf;
      break;
    case 0x9:
      set = !interpreter->sf;
      break;
    case 0xA:
      set = interpreter->pf;
      break;
    case 0xB:
      set = !interpreter->pf;
      break;
    case 0xC:
      set = interpreter->sf != interpreter->of;
      break;
    case 0xD:
      set = interpreter->sf == interpreter->of;
      break;
    case 0xE:
      set = interpreter->zf || (interpreter->sf != interpreter->of);
      break;
    case 0xF:
      set = !interpreter->zf && (interpreter->sf == interpreter->of);
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
  // syscall.s leaves registers in the Linux syscall ABI layout at the syscall
  // instruction: rdi, rsi, rdx, r10, r8, r9.
  int64_t result = X86_64HandleSyscall(
      interpreter, (int64_t)ReadReg(interpreter, X86_REG_RAX),
      (int64_t)ReadReg(interpreter, X86_REG_RDI),
      (int64_t)ReadReg(interpreter, X86_REG_RSI),
      (int64_t)ReadReg(interpreter, X86_REG_RDX),
      (int64_t)ReadReg(interpreter, X86_REG_R10),
      (int64_t)ReadReg(interpreter, X86_REG_R8),
      (int64_t)ReadReg(interpreter, X86_REG_R9));
  WriteReg(interpreter, X86_REG_RAX, (uint64_t)result);
  return true;
}

// Execute a two-byte SSE instruction (the byte after 0x0F is `opcode`, the
// mandatory/legacy prefix is `prefix`).  Returns true if `opcode` is a
// recognized SSE opcode (in which case *ok reports whether execution
// succeeded); returns false for opcodes this handler does not implement so the
// caller can fall through to other 0x0F handlers.
static bool ExecuteSSE(X86_64Interpreter* interpreter, size_t* pos, REX rex,
                       uint8_t prefix, uint8_t opcode, bool* ok) {
  bool is_sd = (prefix == 0xf2);  // scalar double (f2)
  bool is_ss = (prefix == 0xf3);  // scalar single (f3)
  bool is_pd = (prefix == 0x66);  // packed double / 66-prefixed

  switch (opcode) {
    case 0x10:    // movss/movsd/movups load: xmm <- xmm/mem
    case 0x11: {  // movss/movsd/movups store: xmm/mem <- xmm
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      bool store = (opcode == 0x11);
      int xmm = modrm.reg;
      int width = is_ss ? 4 : is_sd ? 8 : 16;
      if (modrm.mod == 3) {
        int other = modrm.rm;
        if (width == 4) {
          uint64_t v = interpreter->xmm[store ? xmm : other][0] & 0xffffffffULL;
          int dst = store ? other : xmm;
          interpreter->xmm[dst][0] =
              (interpreter->xmm[dst][0] & ~0xffffffffULL) | v;
        } else if (width == 8) {
          int src = store ? xmm : other;
          int dst = store ? other : xmm;
          interpreter->xmm[dst][0] = interpreter->xmm[src][0];
        } else {
          int src = store ? xmm : other;
          int dst = store ? other : xmm;
          interpreter->xmm[dst][0] = interpreter->xmm[src][0];
          interpreter->xmm[dst][1] = interpreter->xmm[src][1];
        }
        *ok = true;
        return true;
      }
      uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
      if (store) {
        if (width == 4) {
          Store32(interpreter, addr, (uint32_t)interpreter->xmm[xmm][0]);
        } else if (width == 8) {
          Store64(interpreter, addr, interpreter->xmm[xmm][0]);
        } else {
          Store64(interpreter, addr, interpreter->xmm[xmm][0]);
          Store64(interpreter, addr + 8, interpreter->xmm[xmm][1]);
        }
      } else {
        if (width == 4) {
          interpreter->xmm[xmm][0] = Load32(interpreter, addr);
          interpreter->xmm[xmm][1] = 0;
        } else if (width == 8) {
          interpreter->xmm[xmm][0] = Load64(interpreter, addr);
          interpreter->xmm[xmm][1] = 0;
        } else {
          interpreter->xmm[xmm][0] = Load64(interpreter, addr);
          interpreter->xmm[xmm][1] = Load64(interpreter, addr + 8);
        }
      }
      *ok = true;
      return true;
    }

    case 0x28:    // movaps/movapd: xmm <- xmm/mem (128-bit)
    case 0x29: {  // movaps/movapd: xmm/mem <- xmm (128-bit)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      bool store = (opcode == 0x29);
      int xmm = modrm.reg;
      if (modrm.mod == 3) {
        int other = modrm.rm;
        int src = store ? xmm : other;
        int dst = store ? other : xmm;
        interpreter->xmm[dst][0] = interpreter->xmm[src][0];
        interpreter->xmm[dst][1] = interpreter->xmm[src][1];
      } else {
        uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
        if (store) {
          Store64(interpreter, addr, interpreter->xmm[xmm][0]);
          Store64(interpreter, addr + 8, interpreter->xmm[xmm][1]);
        } else {
          interpreter->xmm[xmm][0] = Load64(interpreter, addr);
          interpreter->xmm[xmm][1] = Load64(interpreter, addr + 8);
        }
      }
      *ok = true;
      return true;
    }

    case 0x2a: {  // cvtsi2ss / cvtsi2sd: xmm <- int gpr/mem
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      int64_t ival;
      if (modrm.mod == 3) {
        ival = rex.w ? (int64_t)ReadReg(interpreter, modrm.rm)
                     : (int64_t)(int32_t)(uint32_t)ReadReg(interpreter, modrm.rm);
      } else {
        uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
        ival = rex.w ? (int64_t)Load64(interpreter, addr)
                     : (int64_t)(int32_t)Load32(interpreter, addr);
      }
      if (is_sd) {
        XmmWriteDouble(interpreter, modrm.reg, (double)ival);
      } else {
        XmmWriteFloat(interpreter, modrm.reg, (float)ival);
      }
      *ok = true;
      return true;
    }

    case 0x2c:    // cvttss2si / cvttsd2si: int gpr <- xmm (truncate)
    case 0x2d: {  // cvtss2si / cvtsd2si: int gpr <- xmm (round)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      double val;
      if (is_sd) {
        val = (modrm.mod == 3)
                  ? XmmReadDouble(interpreter, modrm.rm)
                  : BitsToDouble(Load64(
                        interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
      } else {
        val = (modrm.mod == 3)
                  ? (double)XmmReadFloat(interpreter, modrm.rm)
                  : (double)BitsToFloat(Load32(
                        interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
      }
      if (opcode == 0x2d) {
        val = nearbyint(val);
      } else {
        val = trunc(val);
      }
      uint64_t result =
          rex.w ? (uint64_t)(int64_t)val : (uint64_t)(uint32_t)(int32_t)val;
      WriteReg(interpreter, modrm.reg, result);
      *ok = true;
      return true;
    }

    case 0x2e:    // ucomiss / ucomisd
    case 0x2f: {  // comiss / comisd (same flag behaviour here)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      double a, b;
      if (is_pd) {
        a = XmmReadDouble(interpreter, modrm.reg);
        b = (modrm.mod == 3)
                ? XmmReadDouble(interpreter, modrm.rm)
                : BitsToDouble(Load64(
                      interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
      } else {
        a = (double)XmmReadFloat(interpreter, modrm.reg);
        b = (modrm.mod == 3)
                ? (double)XmmReadFloat(interpreter, modrm.rm)
                : (double)BitsToFloat(Load32(
                      interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
      }
      interpreter->of = false;
      interpreter->sf = false;
      if (isnan(a) || isnan(b)) {
        interpreter->zf = interpreter->pf = interpreter->cf = true;
      } else if (a > b) {
        interpreter->zf = interpreter->pf = interpreter->cf = false;
      } else if (a < b) {
        interpreter->zf = false;
        interpreter->pf = false;
        interpreter->cf = true;
      } else {
        interpreter->zf = true;
        interpreter->pf = false;
        interpreter->cf = false;
      }
      *ok = true;
      return true;
    }

    case 0x51:    // sqrtss / sqrtsd
    case 0x54:    // andps / andpd
    case 0x57:    // xorps / xorpd
    case 0x58:    // addss / addsd
    case 0x59:    // mulss / mulsd
    case 0x5c:    // subss / subsd
    case 0x5d:    // minss / minsd
    case 0x5e:    // divss / divsd
    case 0x5f: {  // maxss / maxsd
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      int dst = modrm.reg;
      // Bitwise logical ops operate on the raw lanes (both 64-bit halves).
      if (opcode == 0x54 || opcode == 0x57) {
        uint64_t lo, hi;
        if (modrm.mod == 3) {
          lo = interpreter->xmm[modrm.rm][0];
          hi = interpreter->xmm[modrm.rm][1];
        } else {
          uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
          lo = Load64(interpreter, addr);
          hi = Load64(interpreter, addr + 8);
        }
        if (opcode == 0x54) {
          interpreter->xmm[dst][0] &= lo;
          interpreter->xmm[dst][1] &= hi;
        } else {
          interpreter->xmm[dst][0] ^= lo;
          interpreter->xmm[dst][1] ^= hi;
        }
        *ok = true;
        return true;
      }
      if (is_sd) {
        double a = XmmReadDouble(interpreter, dst);
        double b;
        if (opcode == 0x51) {
          // sqrt is unary: source is r/m.
          b = (modrm.mod == 3)
                  ? XmmReadDouble(interpreter, modrm.rm)
                  : BitsToDouble(Load64(interpreter,
                                        EffectiveAddress(interpreter, &modrm, *pos)));
          XmmWriteDouble(interpreter, dst, sqrt(b));
          *ok = true;
          return true;
        }
        b = (modrm.mod == 3)
                ? XmmReadDouble(interpreter, modrm.rm)
                : BitsToDouble(Load64(
                      interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
        double r = a;
        switch (opcode) {
          case 0x58: r = a + b; break;
          case 0x59: r = a * b; break;
          case 0x5c: r = a - b; break;
          case 0x5e: r = a / b; break;
          case 0x5d: r = (a < b) ? a : b; break;
          case 0x5f: r = (a > b) ? a : b; break;
        }
        XmmWriteDouble(interpreter, dst, r);
      } else {
        float a = XmmReadFloat(interpreter, dst);
        float b;
        if (opcode == 0x51) {
          b = (modrm.mod == 3)
                  ? XmmReadFloat(interpreter, modrm.rm)
                  : BitsToFloat(Load32(interpreter,
                                       EffectiveAddress(interpreter, &modrm, *pos)));
          XmmWriteFloat(interpreter, dst, sqrtf(b));
          *ok = true;
          return true;
        }
        b = (modrm.mod == 3)
                ? XmmReadFloat(interpreter, modrm.rm)
                : BitsToFloat(Load32(
                      interpreter, EffectiveAddress(interpreter, &modrm, *pos)));
        float r = a;
        switch (opcode) {
          case 0x58: r = a + b; break;
          case 0x59: r = a * b; break;
          case 0x5c: r = a - b; break;
          case 0x5e: r = a / b; break;
          case 0x5d: r = (a < b) ? a : b; break;
          case 0x5f: r = (a > b) ? a : b; break;
        }
        XmmWriteFloat(interpreter, dst, r);
      }
      *ok = true;
      return true;
    }

    case 0x5a: {  // cvtss2sd (f3) / cvtsd2ss (f2)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      if (is_ss) {  // cvtss2sd: float source -> double dest
        float f = (modrm.mod == 3)
                      ? XmmReadFloat(interpreter, modrm.rm)
                      : BitsToFloat(Load32(interpreter,
                                           EffectiveAddress(interpreter, &modrm, *pos)));
        XmmWriteDouble(interpreter, modrm.reg, (double)f);
      } else {  // cvtsd2ss: double source -> float dest
        double d = (modrm.mod == 3)
                       ? XmmReadDouble(interpreter, modrm.rm)
                       : BitsToDouble(Load64(interpreter,
                                             EffectiveAddress(interpreter, &modrm, *pos)));
        XmmWriteFloat(interpreter, modrm.reg, (float)d);
      }
      *ok = true;
      return true;
    }

    case 0x6e: {  // movd/movq: xmm <- gpr/mem (66 prefix; rex.w => 64-bit)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      uint64_t v;
      if (modrm.mod == 3) {
        v = ReadReg(interpreter, modrm.rm);
      } else {
        uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
        v = rex.w ? Load64(interpreter, addr) : Load32(interpreter, addr);
      }
      interpreter->xmm[modrm.reg][0] = rex.w ? v : (uint32_t)v;
      interpreter->xmm[modrm.reg][1] = 0;
      *ok = true;
      return true;
    }

    case 0x7e: {  // movq xmm<-xmm/mem (f3) ; movd/movq xmm->gpr/mem (66)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      if (is_ss) {  // f3 0f 7e: load low 64 bits into xmm, zero the upper lane.
        if (modrm.mod == 3) {
          interpreter->xmm[modrm.reg][0] = interpreter->xmm[modrm.rm][0];
        } else {
          interpreter->xmm[modrm.reg][0] =
              Load64(interpreter, EffectiveAddress(interpreter, &modrm, *pos));
        }
        interpreter->xmm[modrm.reg][1] = 0;
      } else {  // 66 0f 7e: store xmm to gpr/mem.
        uint64_t v = interpreter->xmm[modrm.reg][0];
        if (modrm.mod == 3) {
          WriteReg(interpreter, modrm.rm, rex.w ? v : (uint32_t)v);
        } else {
          uint64_t addr = EffectiveAddress(interpreter, &modrm, *pos);
          if (rex.w) {
            Store64(interpreter, addr, v);
          } else {
            Store32(interpreter, addr, (uint32_t)v);
          }
        }
      }
      *ok = true;
      return true;
    }

    case 0xd6: {  // movq xmm/m64 <- xmm (66 prefix, store form)
      ModRM modrm;
      if (!DecodeModRM(interpreter, pos, rex, true, &modrm)) {
        *ok = false;
        return true;
      }
      uint64_t v = interpreter->xmm[modrm.reg][0];
      if (modrm.mod == 3) {
        interpreter->xmm[modrm.rm][0] = v;
        interpreter->xmm[modrm.rm][1] = 0;
      } else {
        Store64(interpreter, EffectiveAddress(interpreter, &modrm, *pos), v);
      }
      *ok = true;
      return true;
    }

    default:
      return false;
  }
}

static bool ExecuteInstruction(X86_64Interpreter* interpreter, size_t* insn_len,
                               bool* rip_updated) {
  *rip_updated = false;
  interpreter->rip_updated = false;
  interpreter->current_seg_prefix = 0;
  size_t pos = 0;
  uint8_t b0 = Fetch8(interpreter, &pos);

  while (b0 == 0x26 || b0 == 0x2e || b0 == 0x36 || b0 == 0x3e || b0 == 0x64 ||
         b0 == 0x65) {
    interpreter->current_seg_prefix = b0;
    b0 = Fetch8(interpreter, &pos);
  }

  // Consume legacy / mandatory SSE prefixes (0x66 operand-size / packed-double,
  // 0xF2 scalar-double, 0xF3 scalar-single).  These precede any REX prefix.
  uint8_t sse_prefix = 0;
  while (b0 == 0x66 || b0 == 0xf2 || b0 == 0xf3) {
    sse_prefix = b0;
    b0 = Fetch8(interpreter, &pos);
  }

  REX rex = {0};
  if (b0 >= 0x40 && b0 <= 0x4F) {
    rex = ParseRex(interpreter, &pos, b0);
    b0 = Fetch8(interpreter, &pos);
  }

  if (b0 == 0x0F) {
    uint8_t b1 = Fetch8(interpreter, &pos);
    bool sse_ok = false;
    if (ExecuteSSE(interpreter, &pos, rex, sse_prefix, b1, &sse_ok)) {
      *insn_len = pos;
      return sse_ok;
    }
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
        case 0x2:
          take = interpreter->cf;
          break;
        case 0x3:
          take = !interpreter->cf;
          break;
        case 0x4:
          take = interpreter->zf;
          break;
        case 0x5:
          take = !interpreter->zf;
          break;
        case 0x6:
          take = interpreter->cf || interpreter->zf;
          break;
        case 0x7:
          take = !interpreter->cf && !interpreter->zf;
          break;
        case 0x8:
          take = interpreter->sf;
          break;
        case 0x9:
          take = !interpreter->sf;
          break;
        case 0xA:
          take = interpreter->pf;
          break;
        case 0xB:
          take = !interpreter->pf;
          break;
        case 0xC:
          take = interpreter->sf != interpreter->of;
          break;
        case 0xD:
          take = interpreter->sf == interpreter->of;
          break;
        case 0xE:
          take = interpreter->zf || (interpreter->sf != interpreter->of);
          break;
        case 0xF:
          take = !interpreter->zf && (interpreter->sf == interpreter->of);
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
    if (b1 == 0xAF) {  // IMUL r, r/m  (result truncated to operand size)
      ModRM modrm;
      if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
        return false;
      }
      uint64_t src;
      if (modrm.mod == 3) {
        src = ReadReg(interpreter, modrm.rm);
      } else {
        uint64_t addr = EffectiveAddress(interpreter, &modrm, pos);
        src = rex.w ? Load64(interpreter, addr) : Load32(interpreter, addr);
      }
      uint64_t dst = ReadReg(interpreter, modrm.reg);
      uint64_t result;
      if (rex.w) {
        result = (uint64_t)((int64_t)dst * (int64_t)src);
      } else {
        result = (uint32_t)((int32_t)(uint32_t)dst * (int32_t)(uint32_t)src);
      }
      UpdateFlags(interpreter, result, rex.w);
      WriteReg(interpreter, modrm.reg, result);
      *insn_len = pos;
      return true;
    }
    if (b1 == 0xB6 || b1 == 0xB7 || b1 == 0xBE || b1 == 0xBF) {
      // movzx / movsx: B6/BE byte source, B7/BF word source; BE/BF sign-extend.
      ModRM modrm;
      if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
        return false;
      }
      bool byte_src = (b1 == 0xB6 || b1 == 0xBE);
      bool sign = (b1 == 0xBE || b1 == 0xBF);
      uint64_t src;
      if (modrm.mod == 3) {
        src = ReadReg(interpreter, modrm.rm);
      } else {
        uint64_t addr = EffectiveAddress(interpreter, &modrm, pos);
        src = byte_src ? Load8(interpreter, addr) : Load16(interpreter, addr);
      }
      uint64_t result;
      if (byte_src) {
        result = sign ? (uint64_t)(int64_t)(int8_t)(uint8_t)src
                      : (uint64_t)(uint8_t)src;
      } else {
        result = sign ? (uint64_t)(int64_t)(int16_t)(uint16_t)src
                      : (uint64_t)(uint16_t)src;
      }
      if (!rex.w) {
        result = (uint32_t)result;
      }
      WriteReg(interpreter, modrm.reg, result);
      *insn_len = pos;
      return true;
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

  if (b0 == 0x99) {  // CQO (rex.w) / CDQ: sign-extend RAX into RDX
    uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
    if (rex.w) {
      WriteReg(interpreter, X86_REG_RDX,
               (int64_t)rax < 0 ? UINT64_MAX : 0);
    } else {
      WriteReg(interpreter, X86_REG_RDX,
               ((int32_t)(uint32_t)rax < 0) ? 0xffffffffULL : 0);
    }
    *insn_len = pos;
    return true;
  }

  if (b0 == 0x63) {  // MOVSXD: sign-extend r/m32 into a 64-bit register
    ModRM modrm;
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    uint32_t src;
    if (modrm.mod == 3) {
      src = (uint32_t)ReadReg(interpreter, modrm.rm);
    } else {
      uint64_t addr = EffectiveAddress(interpreter, &modrm, pos);
      src = Load32(interpreter, addr);
    }
    uint64_t result = (uint64_t)(int64_t)(int32_t)src;
    if (!rex.w) {
      result = (uint32_t)result;
    }
    WriteReg(interpreter, modrm.reg, result);
    *insn_len = pos;
    return true;
  }

  if (b0 == 0x98) {  // CDQE (rex.w) / CWDE: sign-extend RAX
    uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
    if (rex.w) {
      WriteReg(interpreter, X86_REG_RAX, (uint64_t)(int64_t)(int32_t)(uint32_t)rax);
    } else {
      WriteReg(interpreter, X86_REG_RAX, (uint32_t)(int32_t)(int16_t)(uint16_t)rax);
    }
    *insn_len = pos;
    return true;
  }

  ModRM modrm;
  if (b0 == 0xC7 || b0 == 0xC6) {
    bool byte = (b0 == 0xC6);
    bool word = (sse_prefix == 0x66) && !byte && !rex.w;
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    if (modrm.reg != 0) {
      return false;
    }
    int64_t imm;
    if (byte) {
      imm = (int64_t)(int8_t)Fetch8(interpreter, &pos);
    } else if (word) {
      imm = (int64_t)(int16_t)(uint16_t)(Fetch8(interpreter, &pos) |
                                         ((uint16_t)Fetch8(interpreter, &pos) << 8));
    } else {
      imm = (int64_t)(int32_t)Fetch32(interpreter, &pos);
    }
    *insn_len = pos;
    if (modrm.mod == 3) {
      if (byte) {
        uint64_t cur = ReadReg(interpreter, modrm.rm);
        WriteReg(interpreter, modrm.rm, (cur & ~0xffULL) | (uint64_t)(uint8_t)imm);
      } else if (word) {
        uint64_t cur = ReadReg(interpreter, modrm.rm);
        WriteReg(interpreter, modrm.rm, (cur & ~0xffffULL) | (uint64_t)(uint16_t)imm);
      } else {
        WriteReg(interpreter, modrm.rm,
                 rex.w ? (uint64_t)imm : (uint64_t)(uint32_t)imm);
      }
      return true;
    }
    uint64_t addr = EffectiveAddress(interpreter, &modrm, pos);
    if (byte) {
      Store8(interpreter, addr, (uint8_t)imm);
    } else if (word) {
      Store16(interpreter, addr, (uint16_t)imm);
    } else if (rex.w) {
      Store64(interpreter, addr, (uint64_t)imm);
    } else {
      Store32(interpreter, addr, (uint32_t)imm);
    }
    return true;
  }

  if ((b0 & 0xFE) == 0x88 || (b0 & 0xFE) == 0x8A) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteMovRegMem(interpreter, *insn_len, rex, b0, modrm,
                            sse_prefix == 0x66);
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

  // OR (0x09/0x0B), AND (0x21/0x23) and XOR (0x31/0x33) reg/reg or reg/mem
  // forms.  The "...1" opcode stores into r/m (reg is source); the "...3"
  // opcode has the register as destination.
  if (b0 == 0x09 || b0 == 0x0B || b0 == 0x21 || b0 == 0x23 || b0 == 0x31 ||
      b0 == 0x33) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    *insn_len = pos;
    return ExecuteAluRegMem(interpreter, *insn_len, rex, b0, modrm,
                            (b0 & 2) != 0);
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
    if (!rex.w) {
      value = (uint32_t)value;
    }
    switch (op) {
      case 2: {  // NOT
        uint64_t result = ~value;
        if (!rex.w) {
          result = (uint32_t)result;
        }
        WriteReg(interpreter, modrm.rm, result);
        return true;
      }
      case 3: {  // NEG
        uint64_t result = rex.w ? (uint64_t)(-(int64_t)value)
                                : (uint64_t)(-(int32_t)(uint32_t)value);
        UpdateFlags(interpreter, result, rex.w);
        WriteReg(interpreter, modrm.rm, result);
        return true;
      }
      case 4: {  // MUL (unsigned): RDX:RAX = RAX * r/m
        uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
        if (rex.w) {
          unsigned __int128 product = (unsigned __int128)rax * value;
          WriteReg(interpreter, X86_REG_RAX, (uint64_t)product);
          WriteReg(interpreter, X86_REG_RDX, (uint64_t)(product >> 64));
        } else {
          uint64_t product = (uint64_t)(uint32_t)rax * (uint32_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint32_t)product);
          WriteReg(interpreter, X86_REG_RDX, (uint32_t)(product >> 32));
        }
        return true;
      }
      case 5: {  // IMUL (signed): RDX:RAX = RAX * r/m
        uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
        if (rex.w) {
          __int128 product = (__int128)(int64_t)rax * (int64_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint64_t)product);
          WriteReg(interpreter, X86_REG_RDX, (uint64_t)(product >> 64));
        } else {
          int64_t product =
              (int64_t)(int32_t)(uint32_t)rax * (int32_t)(uint32_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint32_t)product);
          WriteReg(interpreter, X86_REG_RDX, (uint32_t)((uint64_t)product >> 32));
        }
        return true;
      }
      case 6: {  // DIV (unsigned)
        if (value == 0) {
          return false;
        }
        uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
        uint64_t rdx = ReadReg(interpreter, X86_REG_RDX);
        if (rex.w) {
          unsigned __int128 dividend = ((unsigned __int128)rdx << 64) | rax;
          WriteReg(interpreter, X86_REG_RAX, (uint64_t)(dividend / value));
          WriteReg(interpreter, X86_REG_RDX, (uint64_t)(dividend % value));
        } else {
          uint64_t dividend =
              ((uint64_t)(uint32_t)rdx << 32) | (uint32_t)rax;
          uint32_t divisor = (uint32_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint32_t)(dividend / divisor));
          WriteReg(interpreter, X86_REG_RDX, (uint32_t)(dividend % divisor));
        }
        return true;
      }
      case 7: {  // IDIV (signed)
        if (value == 0) {
          return false;
        }
        uint64_t rax = ReadReg(interpreter, X86_REG_RAX);
        uint64_t rdx = ReadReg(interpreter, X86_REG_RDX);
        if (rex.w) {
          __int128 dividend = ((__int128)(int64_t)rdx << 64) | rax;
          int64_t divisor = (int64_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint64_t)(int64_t)(dividend / divisor));
          WriteReg(interpreter, X86_REG_RDX, (uint64_t)(int64_t)(dividend % divisor));
        } else {
          int64_t dividend =
              (int64_t)(((uint64_t)(uint32_t)rdx << 32) | (uint32_t)rax);
          int32_t divisor = (int32_t)(uint32_t)value;
          WriteReg(interpreter, X86_REG_RAX, (uint32_t)(int32_t)(dividend / divisor));
          WriteReg(interpreter, X86_REG_RDX, (uint32_t)(int32_t)(dividend % divisor));
        }
        return true;
      }
      default:
        return false;
    }
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

  if (b0 == 0xC0 || b0 == 0xC1 || b0 == 0xD0 || b0 == 0xD1 || b0 == 0xD2 ||
      b0 == 0xD3) {
    if (!DecodeModRM(interpreter, &pos, rex, true, &modrm)) {
      return false;
    }
    unsigned count;
    if (b0 == 0xC0 || b0 == 0xC1) {
      count = (unsigned)Fetch8(interpreter, &pos);
    } else if (b0 == 0xD0 || b0 == 0xD1) {
      count = 1;
    } else {
      count = (unsigned)(ReadReg(interpreter, X86_REG_RCX) & 0xff);
    }
    *insn_len = pos;
    return ExecuteShift(interpreter, *insn_len, rex, b0, modrm, count);
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

void X86_64InterpreterInitForThread(
    X86_64Interpreter* interpreter, X86_64ProcessRuntime* process,
    X86_64GuestThread* guest_thread, Loader* loader, uint64_t entry_address,
    int argc, char** argv, char* stack, uint64_t fs_base, size_t tls_block_size,
    bool trace_registers, bool trace_instructions) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->process = process;
  interpreter->guest_thread = guest_thread;
  if (guest_thread != NULL) {
    interpreter->guest_tid = guest_thread->tid;
  }
  interpreter->trace_registers = trace_registers;
  interpreter->trace_instructions = trace_instructions;
  if (stack != NULL) {
    interpreter->stack = stack;
    interpreter->owns_stack = false;
  } else {
    interpreter->stack = malloc(X86_64_STACK_SIZE);
    interpreter->owns_stack = true;
  }
  interpreter->rsp =
      (uint64_t)(uintptr_t)(interpreter->stack + X86_64_STACK_SIZE);
  interpreter->rsp &= ~0xFULL;
  if (fs_base != 0) {
    interpreter->fs_base = fs_base;
    interpreter->tls_block_size = tls_block_size;
  } else if (loader->tls.present) {
    interpreter->fs_base = loader->tls.fs_base;
    interpreter->tls_block_size = loader->tls.block_size;
  }
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

void X86_64InterpreterPrepareMain(X86_64Interpreter* interpreter,
                                  uint64_t entry_address, int argc,
                                  char** argv, bool is_static_link) {
  interpreter->rsp =
      (uint64_t)(uintptr_t)(interpreter->stack + X86_64_STACK_SIZE);
  interpreter->rsp &= ~0xFULL;
  Push64(interpreter, 0);
  interpreter->rip = entry_address;
  interpreter->running = true;
  interpreter->rip_updated = false;

  WriteReg(interpreter, X86_REG_RDI, (uint64_t)argc);
  if (!is_static_link) {
    WriteReg(interpreter, X86_REG_RSI, entry_address);
  } else {
    WriteReg(interpreter, X86_REG_RSI, (uint64_t)(uintptr_t)argv);
  }
}

void X86_64InterpreterInit(X86_64Interpreter* interpreter, Loader* loader,
                             uint64_t entry_address, int argc, char** argv,
                             bool trace_registers, bool trace_instructions) {
  X86_64InterpreterInitForThread(interpreter, NULL, NULL, loader, entry_address,
                                 argc, argv, NULL, 0, 0, trace_registers,
                                 trace_instructions);
}

static int X86_64InterpreterRunLoop(X86_64Interpreter* interpreter) {
  while (interpreter->running) {
    if (interpreter->rip == 0) {
      interpreter->exit_code = (int)ReadReg(interpreter, X86_REG_RAX);
      interpreter->running = false;
      break;
    }

    if (interpreter->trace_instructions) {
      SymbolScope* sym =
          X86_64InterpreterFindSymbol(interpreter, interpreter->rip);
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
      X86_64InterpreterFail(interpreter, 1);
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

void X86_64InterpreterPrepareCall(X86_64Interpreter* interpreter, uint64_t fn,
                                  uint64_t arg) {
  interpreter->rsp =
      (uint64_t)(uintptr_t)(interpreter->stack + X86_64_STACK_SIZE);
  interpreter->rsp &= ~0xFULL;
  Push64(interpreter, 0);
  interpreter->rip = fn;
  interpreter->running = true;
  interpreter->rip_updated = false;
  WriteReg(interpreter, X86_REG_RDI, arg);
}

int X86_64InterpreterRunUntilReturn(X86_64Interpreter* interpreter) {
  return X86_64InterpreterRunLoop(interpreter);
}

int X86_64InterpreterCall(X86_64Interpreter* interpreter, uint64_t fn,
                          uint64_t arg) {
  X86_64InterpreterPrepareCall(interpreter, fn, arg);
  return X86_64InterpreterRunUntilReturn(interpreter);
}

int X86_64InterpreterRun(X86_64Interpreter* interpreter) {
  return X86_64InterpreterRunLoop(interpreter);
}

void X86_64InterpreterDestruct(X86_64Interpreter* interpreter) {
  if (interpreter->owns_stack && interpreter->stack != NULL) {
    free(interpreter->stack);
    interpreter->stack = NULL;
  }
}
