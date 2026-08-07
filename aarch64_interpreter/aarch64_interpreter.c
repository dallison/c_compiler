//
//  aarch64_interpreter.c
//  aarch64_interpreter
//

#include "aarch64_interpreter.h"
#include "aarch64_process.h"
#include "aarch64_syscalls.h"
#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
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

// Floating-point register access.  Doubles are stored as their raw 64-bit
// pattern; singles live in the low 32 bits.
static double ReadD(AARCH64Interpreter* interpreter, int reg) {
  double d;
  uint64_t bits = interpreter->v[reg];
  memcpy(&d, &bits, sizeof(d));
  return d;
}

static void WriteD(AARCH64Interpreter* interpreter, int reg, double value) {
  uint64_t bits;
  memcpy(&bits, &value, sizeof(bits));
  interpreter->v[reg] = bits;
}

static float ReadS(AARCH64Interpreter* interpreter, int reg) {
  float f;
  uint32_t bits = (uint32_t)interpreter->v[reg];
  memcpy(&f, &bits, sizeof(f));
  return f;
}

static void WriteS(AARCH64Interpreter* interpreter, int reg, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  interpreter->v[reg] = bits;  // Upper 32 bits cleared.
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

static bool ProcessMemoryOk(AARCH64Interpreter* interpreter, uint64_t addr,
                            size_t size) {
  if (interpreter->process == NULL) {
    return false;
  }
  return AARCH64ProcessGuestMemoryOk(interpreter->process, addr, size);
}

static bool InterpreterAddressOk(AARCH64Interpreter* interpreter, uint64_t addr,
                                 size_t size) {
  if (interpreter->tp_base != 0 && interpreter->tls_block_size > 0) {
    uint64_t tls_start = interpreter->tp_base;
    uint64_t tls_end = tls_start + (uint64_t)interpreter->tls_block_size;
    if (addr >= tls_start && addr + size <= tls_end) {
      return true;
    }
  }
  if (interpreter->stack != NULL) {
    uint64_t start = (uint64_t)(uintptr_t)interpreter->stack;
    uint64_t end = start + AARCH64_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  if (ProcessMemoryOk(interpreter, addr, size)) {
    return true;
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

static void GuestMemoryLock(AARCH64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->memory_mutex);
  }
}

static void GuestMemoryUnlock(AARCH64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->memory_mutex);
  }
}

static void GuestMemoryDidWrite(AARCH64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    interpreter->process->write_epoch++;
  }
  interpreter->reservation_valid = false;
}

static void Store64(AARCH64Interpreter* interpreter, uint64_t addr,
                    uint64_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Store64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  *(uint64_t*)(uintptr_t)addr = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static void Store32(AARCH64Interpreter* interpreter, uint64_t addr,
                    uint32_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  *(uint32_t*)(uintptr_t)addr = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static uint64_t Load64(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 8)) {
    fprintf(stderr, "Load64 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  uint64_t value = *(uint64_t*)(uintptr_t)addr;
  GuestMemoryUnlock(interpreter);
  return value;
}

static uint32_t Load32(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  uint32_t value = *(uint32_t*)(uintptr_t)addr;
  GuestMemoryUnlock(interpreter);
  return value;
}

static void Store16(AARCH64Interpreter* interpreter, uint64_t addr,
                    uint16_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Store16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  *(uint16_t*)(uintptr_t)addr = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static void Store8(AARCH64Interpreter* interpreter, uint64_t addr,
                   uint8_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Store8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  *(uint8_t*)(uintptr_t)addr = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static uint16_t Load16(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Load16 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  uint16_t value = *(uint16_t*)(uintptr_t)addr;
  GuestMemoryUnlock(interpreter);
  return value;
}

static uint8_t Load8(AARCH64Interpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Load8 outside mapped memory at 0x%" PRIx64 "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }
  GuestMemoryLock(interpreter);
  uint8_t value = *(uint8_t*)(uintptr_t)addr;
  GuestMemoryUnlock(interpreter);
  return value;
}

static int64_t SignExtend64(uint64_t value, int bits) {
  uint64_t sign_bit = 1ULL << (bits - 1);
  return (int64_t)((value ^ sign_bit) - sign_bit);
}

// Computes NZCV for an add/subtract, mirroring AArch64 AddWithCarry. For
// subtraction the caller passes the operand already complemented and carry_in
// set to 1 (i.e. lhs + ~rhs + 1).
static void SetAddSubFlags(AARCH64Interpreter* interpreter, uint64_t lhs,
                           uint64_t rhs, uint64_t carry_in, bool sf,
                           uint64_t* out_result) {
  uint64_t result;
  bool carry;
  bool overflow;
  if (sf) {
    uint64_t s1 = lhs + rhs;
    bool c1 = s1 < lhs;
    result = s1 + carry_in;
    bool c2 = result < s1;
    carry = c1 || c2;
    overflow = (~(lhs ^ rhs) & (lhs ^ result) & 0x8000000000000000ULL) != 0;
  } else {
    lhs &= 0xffffffffULL;
    rhs &= 0xffffffffULL;
    uint64_t sum = lhs + rhs + carry_in;
    result = sum & 0xffffffffULL;
    carry = (sum >> 32) & 1;
    overflow = (~(lhs ^ rhs) & (lhs ^ result) & 0x80000000ULL) != 0;
  }
  interpreter->flag_n =
      sf ? ((result >> 63) & 1) : ((result >> 31) & 1);
  interpreter->flag_z = (result == 0);
  interpreter->flag_c = carry;
  interpreter->flag_v = overflow;
  if (out_result != NULL) {
    *out_result = result;
  }
}

static bool EvaluateCondition(AARCH64Interpreter* interpreter, int cond) {
  bool n = interpreter->flag_n;
  bool z = interpreter->flag_z;
  bool c = interpreter->flag_c;
  bool v = interpreter->flag_v;
  bool result;
  switch (cond >> 1) {
    case 0: result = z; break;            // EQ/NE
    case 1: result = c; break;            // CS/CC
    case 2: result = n; break;            // MI/PL
    case 3: result = v; break;            // VS/VC
    case 4: result = c && !z; break;      // HI/LS
    case 5: result = n == v; break;       // GE/LT
    case 6: result = (n == v) && !z; break;  // GT/LE
    default: result = true; break;        // AL/NV
  }
  if ((cond & 1) && (cond != 0xf)) {
    result = !result;
  }
  return result;
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

static uint64_t LowOnes(int n);

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
  // N bit (bit 21) selects the inverted-operand forms (BIC/ORN/EON/BICS).
  if ((insn >> 21) & 1) {
    rhs = ~rhs;
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
      result = lhs & rhs;  // ANDS/BICS - sets flags below.
      break;
    default:
      return false;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  if (opc == 3) {
    interpreter->flag_n = (result >> (sf ? 63 : 31)) & 1;
    interpreter->flag_z = (result == 0);
    interpreter->flag_c = false;
    interpreter->flag_v = false;
  }
  WriteX(interpreter, rd, result);
  return true;
}

// Decode the AArch64 logical-immediate bitmask (N:imms:immr) into a constant.
static bool DecodeBitMasks(bool sf, int n, int imms, int immr, uint64_t* out) {
  int len = 31 - __builtin_clz((n << 6) | ((~imms) & 0x3f));
  if (len < 1) {
    return false;
  }
  int esize = 1 << len;
  if (!sf && esize > 32) {
    return false;
  }
  int levels = esize - 1;
  int s = imms & levels;
  int r = immr & levels;
  if (s == levels) {
    return false;
  }
  uint64_t welem = LowOnes(s + 1);
  // Rotate welem right by r within esize bits.
  uint64_t rotated =
      ((welem >> r) | (welem << (esize - r))) & LowOnes(esize);
  // Replicate across 64 bits.
  uint64_t result = 0;
  for (int i = 0; i < 64; i += esize) {
    result |= rotated << i;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  *out = result;
  return true;
}

// Logical (immediate): AND/ORR/EOR/ANDS with a bitmask immediate.
static bool ExecuteLogicalImm(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int opc = (insn >> 29) & 3;
  int n = (insn >> 22) & 1;
  int immr = (insn >> 16) & 0x3f;
  int imms = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  if (!sf && n != 0) {
    return false;
  }
  uint64_t imm;
  if (!DecodeBitMasks(sf, n, imms, immr, &imm)) {
    return false;
  }
  uint64_t lhs = ReadX(interpreter, rn);
  if (!sf) {
    lhs &= 0xffffffff;
  }
  uint64_t result = 0;
  switch (opc) {
    case 0:
    case 3:
      result = lhs & imm;
      break;
    case 1:
      result = lhs | imm;
      break;
    case 2:
      result = lhs ^ imm;
      break;
  }
  if (!sf) {
    result &= 0xffffffff;
  }
  if (opc == 3) {
    interpreter->flag_n = (result >> (sf ? 63 : 31)) & 1;
    interpreter->flag_z = (result == 0);
    interpreter->flag_c = false;
    interpreter->flag_v = false;
  }
  // ANDS with rd==31 is the TST alias and must not write SP; AND with rd==31
  // writes SP.  We only model the GP write here (rd 31 -> xzr drops it).
  WriteX(interpreter, rd, result);
  return true;
}

// Conditional select: CSEL/CSINC/CSINV/CSNEG (and aliases cset/csinc/etc.).
static bool ExecuteCondSelect(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int op = (insn >> 30) & 1;     // bit30: selects INV/NEG family.
  int rm = (insn >> 16) & 0x1f;
  int cond = (insn >> 12) & 0xf;
  int op2 = (insn >> 10) & 3;    // bits[11:10].
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  uint64_t a = ReadX(interpreter, rn);
  uint64_t b = ReadX(interpreter, rm);
  uint64_t result;
  if (EvaluateCondition(interpreter, cond)) {
    result = a;
  } else {
    // else value depends on (op, op2): CSEL=b, CSINC=b+1, CSINV=~b, CSNEG=-b.
    if (op == 0 && op2 == 0) {
      result = b;            // CSEL
    } else if (op == 0 && op2 == 1) {
      result = b + 1;        // CSINC
    } else if (op == 1 && op2 == 0) {
      result = ~b;           // CSINV
    } else if (op == 1 && op2 == 1) {
      result = -b;           // CSNEG
    } else {
      return false;
    }
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
  uint64_t result;
  if (s) {
    if (op) {
      SetAddSubFlags(interpreter, lhs, ~rhs, 1, sf, &result);
    } else {
      SetAddSubFlags(interpreter, lhs, rhs, 0, sf, &result);
    }
    WriteX(interpreter, rd, result);
    return true;
  }
  result = op ? (lhs - rhs) : (lhs + rhs);
  if (!sf) {
    result &= 0xffffffff;
  }
  WriteX(interpreter, rd, result);
  return true;
}

static uint64_t LowOnes(int n) {
  return (n >= 64) ? ~0ULL : ((1ULL << n) - 1);
}

// SBFM/BFM/UBFM (opc 0/1/2). These back the asr/lsr/lsl/sbfx/ubfx/bfi aliases.
static bool ExecuteBitfieldMove(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int opc = (insn >> 29) & 3;
  int immr = (insn >> 16) & 0x3f;
  int imms = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  int width = sf ? 64 : 32;
  uint64_t src = ReadX(interpreter, rn);
  if (!sf) {
    src &= 0xffffffffULL;
  }

  int pos;
  int copylen;
  uint64_t field;
  if (imms >= immr) {
    pos = 0;
    copylen = imms - immr + 1;
    field = (src >> immr) & LowOnes(copylen);
  } else {
    pos = width - immr;
    copylen = imms + 1;
    field = src & LowOnes(copylen);
  }
  uint64_t placed = field << pos;
  uint64_t writemask = LowOnes(copylen) << pos;

  uint64_t result;
  if (opc == 2) {  // UBFM: zeros outside the field.
    result = placed;
  } else if (opc == 1) {  // BFM: keep destination bits outside the field.
    uint64_t dst = ReadX(interpreter, rd);
    if (!sf) {
      dst &= 0xffffffffULL;
    }
    result = (dst & ~writemask) | (placed & writemask);
  } else {  // SBFM: sign extend from the top of the field, zeros below.
    result = placed & writemask;
    int topbit = pos + copylen - 1;
    if ((result >> topbit) & 1) {
      result |= ~LowOnes(topbit + 1);
    }
  }
  if (!sf) {
    result &= 0xffffffffULL;
  }
  WriteX(interpreter, rd, result);
  return true;
}

// Data-processing (2 source): UDIV/SDIV and the variable shifts.
static bool ExecuteDataProc2(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int op = (insn >> 10) & 0x3f;
  int rm = (insn >> 16) & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  int width = sf ? 64 : 32;
  uint64_t n = ReadX(interpreter, rn);
  uint64_t m = ReadX(interpreter, rm);
  if (!sf) {
    n &= 0xffffffffULL;
    m &= 0xffffffffULL;
  }
  uint64_t result;
  switch (op) {
    case 0x02:  // UDIV
      result = (m == 0) ? 0 : (n / m);
      break;
    case 0x03: {  // SDIV
      int64_t sn = sf ? (int64_t)n : (int64_t)(int32_t)n;
      int64_t sm = sf ? (int64_t)m : (int64_t)(int32_t)m;
      result = (sm == 0) ? 0 : (uint64_t)(sn / sm);
      break;
    }
    case 0x08:  // LSLV
      result = n << (m % width);
      break;
    case 0x09:  // LSRV
      result = n >> (m % width);
      break;
    case 0x0a: {  // ASRV
      int sh = (int)(m % width);
      int64_t sn = sf ? (int64_t)n : (int64_t)(int32_t)n;
      result = (uint64_t)(sn >> sh);
      break;
    }
    case 0x0b: {  // RORV
      int sh = (int)(m % width);
      result = (sh == 0) ? n : ((n >> sh) | (n << (width - sh)));
      break;
    }
    default:
      return false;
  }
  if (!sf) {
    result &= 0xffffffffULL;
  }
  WriteX(interpreter, rd, result);
  return true;
}

static bool ExecuteDataProc1(AARCH64Interpreter* interpreter, uint32_t insn) {
  bool sf = (insn >> 31) & 1;
  int op = (insn >> 10) & 0x3f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  int width = sf ? 64 : 32;
  uint64_t value = ReadX(interpreter, rn);
  if (!sf) {
    value = (uint32_t)value;
  }
  uint64_t result;
  if (op == 0) {  // RBIT
    result = 0;
    for (int i = 0; i < width; i++) {
      result = (result << 1) | ((value >> i) & 1);
    }
  } else if (op == 4) {  // CLZ
    result = value == 0
                 ? (uint64_t)width
                 : (sf ? (uint64_t)__builtin_clzll(value)
                       : (uint64_t)__builtin_clz((uint32_t)value));
  } else {
    return false;
  }
  WriteX(interpreter, rd, result);
  return true;
}

// Data-processing (3 source): MADD/MSUB (and thus MUL/MNEG).
static bool ExecuteDataProc3(AARCH64Interpreter* interpreter, uint32_t insn) {
  if (((insn >> 21) & 7) != 0) {
    return false;  // Only the plain MADD/MSUB (op31 == 0) are handled.
  }
  bool sf = (insn >> 31) & 1;
  int rm = (insn >> 16) & 0x1f;
  bool sub = (insn >> 15) & 1;
  int ra = (insn >> 10) & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rd = insn & 0x1f;
  uint64_t n = ReadX(interpreter, rn);
  uint64_t m = ReadX(interpreter, rm);
  uint64_t a = ReadX(interpreter, ra);
  if (!sf) {
    n &= 0xffffffffULL;
    m &= 0xffffffffULL;
    a &= 0xffffffffULL;
  }
  uint64_t prod = n * m;
  uint64_t result = sub ? (a - prod) : (a + prod);
  if (!sf) {
    result &= 0xffffffffULL;
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
  uint64_t result;
  if (s) {
    if (op) {
      SetAddSubFlags(interpreter, lhs, ~imm, 1, sf, &result);
    } else {
      SetAddSubFlags(interpreter, lhs, imm, 0, sf, &result);
    }
    // Flag-setting forms use the zero register, not SP, as destination.
    WriteX(interpreter, rd, result);
    return true;
  }
  result = op ? (lhs - imm) : (lhs + imm);
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

static bool ExecuteCondBranch(AARCH64Interpreter* interpreter, uint32_t insn,
                              bool* pc_updated) {
  int cond = insn & 0xf;
  int64_t imm19 = SignExtend64((insn >> 5) & 0x7ffff, 19);
  if (EvaluateCondition(interpreter, cond)) {
    interpreter->pc = (uint64_t)((int64_t)interpreter->pc + (imm19 << 2));
    *pc_updated = true;
  }
  return true;
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

static uint64_t LoadGuestRaw(uint64_t addr, size_t size) {
  switch (size) {
    case 1: return *(uint8_t*)(uintptr_t)addr;
    case 2: return *(uint16_t*)(uintptr_t)addr;
    case 4: return *(uint32_t*)(uintptr_t)addr;
    default: return *(uint64_t*)(uintptr_t)addr;
  }
}

static void StoreGuestRaw(uint64_t addr, size_t size, uint64_t value) {
  switch (size) {
    case 1: *(uint8_t*)(uintptr_t)addr = (uint8_t)value; break;
    case 2: *(uint16_t*)(uintptr_t)addr = (uint16_t)value; break;
    case 4: *(uint32_t*)(uintptr_t)addr = (uint32_t)value; break;
    default: *(uint64_t*)(uintptr_t)addr = value; break;
  }
}

static bool ExecuteLoadStoreExclusive(AARCH64Interpreter* interpreter,
                                      uint32_t insn) {
  int size_log2 = (insn >> 30) & 3;
  size_t size = (size_t)1 << size_log2;
  bool load = ((insn >> 22) & 1) != 0;
  bool ordered = ((insn >> 15) & 1) != 0;
  int rs = (insn >> 16) & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;
  uint64_t addr = ReadX(interpreter, rn);
  if (!InterpreterAddressOk(interpreter, addr, size)) {
    fprintf(stderr, "Exclusive access outside mapped memory at 0x%" PRIx64
                    "\n", addr);
    AARCH64InterpreterFail(interpreter, 1);
  }

  if (load) {
    GuestMemoryLock(interpreter);
    uint64_t value = LoadGuestRaw(addr, size);
    interpreter->reservation_valid = true;
    interpreter->reservation_address = addr;
    interpreter->reservation_size = (uint32_t)size;
    interpreter->reservation_epoch =
        interpreter->process != NULL ? interpreter->process->write_epoch : 0;
    GuestMemoryUnlock(interpreter);
    if (ordered) {
      atomic_thread_fence(memory_order_acquire);
    }
    WriteX(interpreter, rt, value);
    return true;
  }

  if (ordered) {
    atomic_thread_fence(memory_order_release);
  }
  GuestMemoryLock(interpreter);
  uint64_t epoch =
      interpreter->process != NULL ? interpreter->process->write_epoch : 0;
  bool success = interpreter->reservation_valid &&
                 interpreter->reservation_address == addr &&
                 interpreter->reservation_size == size &&
                 interpreter->reservation_epoch == epoch;
  if (success) {
    StoreGuestRaw(addr, size, ReadX(interpreter, rt));
    if (interpreter->process != NULL) {
      interpreter->process->write_epoch++;
    }
  }
  interpreter->reservation_valid = false;
  GuestMemoryUnlock(interpreter);
  WriteX(interpreter, rs, success ? 0 : 1);
  return true;
}

static bool ExecuteAcquireRelease(AARCH64Interpreter* interpreter,
                                  uint32_t insn) {
  int size_log2 = (insn >> 30) & 3;
  size_t size = (size_t)1 << size_log2;
  bool load = ((insn >> 22) & 1) != 0;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;
  uint64_t addr = ReadX(interpreter, rn);
  if (load) {
    uint64_t value;
    switch (size) {
      case 1: value = Load8(interpreter, addr); break;
      case 2: value = Load16(interpreter, addr); break;
      case 4: value = Load32(interpreter, addr); break;
      default: value = Load64(interpreter, addr); break;
    }
    atomic_thread_fence(memory_order_acquire);
    WriteX(interpreter, rt, value);
  } else {
    atomic_thread_fence(memory_order_release);
    uint64_t value = ReadX(interpreter, rt);
    switch (size) {
      case 1: Store8(interpreter, addr, (uint8_t)value); break;
      case 2: Store16(interpreter, addr, (uint16_t)value); break;
      case 4: Store32(interpreter, addr, (uint32_t)value); break;
      default: Store64(interpreter, addr, value); break;
    }
  }
  return true;
}

static bool ExecuteBarrier(AARCH64Interpreter* interpreter, uint32_t insn) {
  (void)interpreter;
  int option = (insn >> 8) & 0xf;
  if (option == 0x9) {
    atomic_thread_fence(memory_order_acquire);
  } else if (option == 0xa) {
    atomic_thread_fence(memory_order_release);
  } else {
    atomic_thread_fence(memory_order_seq_cst);
  }
  return true;
}

// Handles the integer load/store register forms: unsigned scaled 12-bit
// immediate (0x39 group) as well as the unscaled, pre-/post-indexed and
// register-offset forms (0x38 group), for byte/half/word/dword sizes with
// optional sign extension.
static bool ExecuteLoadStoreImm(AARCH64Interpreter* interpreter,
                                uint32_t insn) {
  bool is_fp = ((insn >> 26) & 1) != 0;
  int size = (insn >> 30) & 3;
  int opc = (insn >> 22) & 3;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;

  uint64_t base = ReadSp(interpreter, rn);
  uint64_t addr;
  bool writeback = false;
  uint64_t wb_value = 0;

  if ((insn >> 24) & 1) {
    // Unsigned scaled 12-bit immediate offset.
    uint64_t imm12 = (insn >> 10) & 0xfff;
    addr = base + (imm12 << size);
  } else if ((insn >> 21) & 1) {
    // Register offset, with optional extend/shift.
    int rm = (insn >> 16) & 0x1f;
    int option = (insn >> 13) & 7;
    int s = (insn >> 12) & 1;
    uint64_t off = ReadX(interpreter, rm);
    if (option == 2) {
      off = (uint32_t)off;  // UXTW
    } else if (option == 6) {
      off = (uint64_t)(int64_t)(int32_t)off;  // SXTW
    }
    if (s) {
      off <<= size;
    }
    addr = base + off;
  } else {
    // Unscaled / pre-index / post-index 9-bit signed immediate.
    int64_t imm9 = SignExtend64((insn >> 12) & 0x1ff, 9);
    int mode = (insn >> 10) & 3;
    if (mode == 1) {  // Post-index.
      addr = base;
      wb_value = (uint64_t)((int64_t)base + imm9);
      writeback = true;
    } else if (mode == 3) {  // Pre-index.
      addr = (uint64_t)((int64_t)base + imm9);
      wb_value = addr;
      writeback = true;
    } else {  // Unscaled (STUR/LDUR).
      addr = (uint64_t)((int64_t)base + imm9);
    }
  }

  if (is_fp) {
    // FP/SIMD scalar load/store.  opc bit 0 selects load(1)/store(0).
    bool load = (opc & 1) != 0;
    if (load) {
      uint64_t value;
      switch (size) {
        case 2: value = Load32(interpreter, addr); break;
        default: value = Load64(interpreter, addr); break;
      }
      interpreter->v[rt] = value;
    } else {
      uint64_t value = interpreter->v[rt];
      switch (size) {
        case 2: Store32(interpreter, addr, (uint32_t)value); break;
        default: Store64(interpreter, addr, value); break;
      }
    }
  } else if (opc == 0) {
    uint64_t value = ReadX(interpreter, rt);
    switch (size) {
      case 0: Store8(interpreter, addr, (uint8_t)value); break;
      case 1: Store16(interpreter, addr, (uint16_t)value); break;
      case 2: Store32(interpreter, addr, (uint32_t)value); break;
      default: Store64(interpreter, addr, value); break;
    }
  } else {
    uint64_t value;
    switch (size) {
      case 0: value = Load8(interpreter, addr); break;
      case 1: value = Load16(interpreter, addr); break;
      case 2: value = Load32(interpreter, addr); break;
      default: value = Load64(interpreter, addr); break;
    }
    if ((opc == 2 || opc == 3) && size < 3) {
      // Signed load: sign extend from the accessed width.
      value = (uint64_t)SignExtend64(value, 8 << size);
      if (opc == 3) {
        value &= 0xffffffffULL;  // Sign extend to 32 bits only.
      }
    }
    WriteX(interpreter, rt, value);
  }

  if (writeback) {
    WriteSp(interpreter, rn, wb_value);
  }
  return true;
}

// Handles STP/LDP for general-purpose registers in all three addressing
// forms (post-index, signed offset and pre-index). The addressing mode lives
// in bits[24:23] (1=post, 2=offset, 3=pre); bits[31:30] give the access size
// (2 => 64-bit, otherwise 32-bit).
static bool ExecuteLoadStorePair(AARCH64Interpreter* interpreter, uint32_t insn) {
  if ((insn >> 26) & 1) {
    return false;  // FP/SIMD pair not supported here.
  }
  int opc = (insn >> 30) & 3;
  int mode = (insn >> 23) & 3;
  bool load = ((insn >> 22) & 1) != 0;
  int64_t imm7 = SignExtend64((insn >> 15) & 0x7f, 7);
  int rt2 = (insn >> 10) & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rt = insn & 0x1f;
  bool is64 = (opc == 2);
  int scale = is64 ? 8 : 4;
  int64_t offset = imm7 * scale;
  bool post = (mode == 1);
  bool writeback = (mode == 1 || mode == 3);

  uint64_t base = ReadSp(interpreter, rn);
  uint64_t addr = post ? base : (uint64_t)((int64_t)base + offset);

  if (is64) {
    if (load) {
      WriteX(interpreter, rt, Load64(interpreter, addr));
      WriteX(interpreter, rt2, Load64(interpreter, addr + 8));
    } else {
      Store64(interpreter, addr, ReadX(interpreter, rt));
      Store64(interpreter, addr + 8, ReadX(interpreter, rt2));
    }
  } else {
    if (load) {
      WriteX(interpreter, rt, Load32(interpreter, addr));
      WriteX(interpreter, rt2, Load32(interpreter, addr + 4));
    } else {
      Store32(interpreter, addr, (uint32_t)ReadX(interpreter, rt));
      Store32(interpreter, addr + 4, (uint32_t)ReadX(interpreter, rt2));
    }
  }

  if (writeback) {
    WriteSp(interpreter, rn, (uint64_t)((int64_t)base + offset));
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

static bool ExecuteAdr(AARCH64Interpreter* interpreter, uint32_t insn) {
  int rd = insn & 0x1f;
  int64_t immhi = (insn >> 5) & 0x7ffff;
  int64_t immlo = (insn >> 29) & 3;
  int64_t imm = SignExtend64((immhi << 2) | immlo, 21);
  WriteX(interpreter, rd, (uint64_t)((int64_t)interpreter->pc + imm));
  return true;
}

static bool ExecuteLoadLiteral(AARCH64Interpreter* interpreter, uint32_t insn) {
  int rt = insn & 0x1f;
  int64_t imm19 = SignExtend64((insn >> 5) & 0x7ffff, 19);
  uint64_t addr = (interpreter->pc & ~3ULL) + (uint64_t)(imm19 << 2);
  WriteX(interpreter, rt, Load64(interpreter, addr));
  return true;
}

// Set NZCV per the AArch64 floating-point compare semantics.
static void SetFPCompareFlags(AARCH64Interpreter* interpreter, double a,
                              double b) {
  if (a < b) {
    interpreter->flag_n = true;
    interpreter->flag_z = false;
    interpreter->flag_c = false;
    interpreter->flag_v = false;
  } else if (a > b) {
    interpreter->flag_n = false;
    interpreter->flag_z = false;
    interpreter->flag_c = true;
    interpreter->flag_v = false;
  } else if (a == b) {
    interpreter->flag_n = false;
    interpreter->flag_z = true;
    interpreter->flag_c = true;
    interpreter->flag_v = false;
  } else {
    // Unordered (a NaN operand).
    interpreter->flag_n = false;
    interpreter->flag_z = false;
    interpreter->flag_c = true;
    interpreter->flag_v = true;
  }
}

// Scalar floating-point data-processing, comparisons and conversions to/from
// the integer register file.  Covers the subset emitted by the DaveCC
// aarch64 backend (single and double precision).
static bool ExecuteFP(AARCH64Interpreter* interpreter, uint32_t insn) {
  int ftype = (insn >> 22) & 3;       // 00 = single, 01 = double.
  bool isD = (ftype == 1);
  int rd = insn & 0x1f;
  int rn = (insn >> 5) & 0x1f;
  int rm = (insn >> 16) & 0x1f;
  int op21 = (insn >> 21) & 1;
  int bits15_10 = (insn >> 10) & 0x3f;

  if (ftype != 0 && ftype != 1) {
    return false;  // Half precision / unsupported.
  }

  if (op21 && bits15_10 == 0) {
    // Conversion between FP and integer, or FMOV bitcast.
    int sf = (insn >> 31) & 1;
    int rmode = (insn >> 19) & 3;
    int opcode = (insn >> 16) & 7;
    switch (opcode) {
      case 2: {  // SCVTF (signed int -> FP).
        int64_t v = sf ? (int64_t)ReadX(interpreter, rn)
                       : (int64_t)(int32_t)(uint32_t)ReadX(interpreter, rn);
        if (isD) WriteD(interpreter, rd, (double)v);
        else WriteS(interpreter, rd, (float)v);
        return true;
      }
      case 3: {  // UCVTF (unsigned int -> FP).
        uint64_t v = sf ? ReadX(interpreter, rn)
                        : (uint32_t)ReadX(interpreter, rn);
        if (isD) WriteD(interpreter, rd, (double)v);
        else WriteS(interpreter, rd, (float)v);
        return true;
      }
      case 0:    // FCVTNS / FCVTZS (FP -> signed int).
      case 4: {  // FCVTAS etc; treat as truncating signed conversion.
        double v = isD ? ReadD(interpreter, rn) : (double)ReadS(interpreter, rn);
        int64_t r = (int64_t)v;
        WriteX(interpreter, rd, sf ? (uint64_t)r : (uint32_t)(int32_t)r);
        return true;
      }
      case 1:
      case 5: {  // FCVTNU etc (FP -> unsigned int).
        double v = isD ? ReadD(interpreter, rn) : (double)ReadS(interpreter, rn);
        uint64_t r = (uint64_t)v;
        WriteX(interpreter, rd, sf ? r : (uint32_t)r);
        return true;
      }
      case 6: {  // FMOV FP -> GP (bitcast).
        WriteX(interpreter, rd,
               isD ? interpreter->v[rn] : (uint32_t)interpreter->v[rn]);
        return true;
      }
      case 7: {  // FMOV GP -> FP (bitcast).
        uint64_t bits = ReadX(interpreter, rn);
        interpreter->v[rd] = isD ? bits : (uint32_t)bits;
        return true;
      }
      default:
        (void)rmode;
        return false;
    }
  }

  if (op21 && ((insn >> 10) & 3) == 2) {
    // FP data-processing (2 source).
    int opcode = (insn >> 12) & 0xf;
    if (isD) {
      double a = ReadD(interpreter, rn);
      double b = ReadD(interpreter, rm);
      double r;
      switch (opcode) {
        case 0x0: r = a * b; break;   // FMUL
        case 0x1: r = a / b; break;   // FDIV
        case 0x2: r = a + b; break;   // FADD
        case 0x3: r = a - b; break;   // FSUB
        case 0x4: r = a > b ? a : b; break;  // FMAX
        case 0x5: r = a < b ? a : b; break;  // FMIN
        default: return false;
      }
      WriteD(interpreter, rd, r);
    } else {
      float a = ReadS(interpreter, rn);
      float b = ReadS(interpreter, rm);
      float r;
      switch (opcode) {
        case 0x0: r = a * b; break;
        case 0x1: r = a / b; break;
        case 0x2: r = a + b; break;
        case 0x3: r = a - b; break;
        case 0x4: r = a > b ? a : b; break;
        case 0x5: r = a < b ? a : b; break;
        default: return false;
      }
      WriteS(interpreter, rd, r);
    }
    return true;
  }

  if (op21 && ((insn >> 10) & 3) == 0 && ((insn >> 14) & 1)) {
    // FP data-processing (1 source): FMOV/FABS/FNEG/FSQRT/FCVT.
    int opcode = (insn >> 15) & 0x3f;
    switch (opcode) {
      case 0x0:  // FMOV (register).
        interpreter->v[rd] = isD ? interpreter->v[rn]
                                 : (uint32_t)interpreter->v[rn];
        return true;
      case 0x1: {  // FABS.
        if (isD) WriteD(interpreter, rd, fabs(ReadD(interpreter, rn)));
        else WriteS(interpreter, rd, fabsf(ReadS(interpreter, rn)));
        return true;
      }
      case 0x2: {  // FNEG.
        if (isD) WriteD(interpreter, rd, -ReadD(interpreter, rn));
        else WriteS(interpreter, rd, -ReadS(interpreter, rn));
        return true;
      }
      case 0x3: {  // FSQRT.
        if (isD) WriteD(interpreter, rd, sqrt(ReadD(interpreter, rn)));
        else WriteS(interpreter, rd, sqrtf(ReadS(interpreter, rn)));
        return true;
      }
      case 0x4:    // FCVT to single (from double).
        WriteS(interpreter, rd, (float)ReadD(interpreter, rn));
        return true;
      case 0x5:    // FCVT to double (from single).
        WriteD(interpreter, rd, (double)ReadS(interpreter, rn));
        return true;
      default:
        return false;
    }
  }

  if (op21 && ((insn >> 10) & 0xf) == 0x8) {
    // FP compare (FCMP / FCMPE).  The second operand is rm, or zero when the
    // "compare with zero" variant (bit 3 of the opcode2 field) is set.
    bool with_zero = ((insn >> 3) & 1) != 0;
    double a = isD ? ReadD(interpreter, rn) : (double)ReadS(interpreter, rn);
    double b = with_zero ? 0.0
                         : (isD ? ReadD(interpreter, rm)
                                : (double)ReadS(interpreter, rm));
    SetFPCompareFlags(interpreter, a, b);
    return true;
  }

  return false;
}

static bool ExecuteInstruction(AARCH64Interpreter* interpreter, uint32_t insn,
                               bool* pc_updated) {
  *pc_updated = false;
  if ((insn & 0xffffffe0u) == 0xd53bd040u) {
    WriteX(interpreter, insn & 31, interpreter->tp_base);
    return true;
  }
  if ((insn & 0xFF000000) == 0x58000000) {
    return ExecuteLoadLiteral(interpreter, insn);
  }
  if ((insn & 0x9F000000) == 0x90000000) {
    return ExecuteAdrp(interpreter, insn);
  }
  if ((insn & 0x9F000000) == 0x10000000) {
    return ExecuteAdr(interpreter, insn);
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
  if ((insn & 0x1F000000) == 0x0A000000) {
    return ExecuteLogicalShifted(interpreter, insn);
  }
  if ((insn & 0x1F800000) == 0x12000000) {
    return ExecuteLogicalImm(interpreter, insn);
  }
  if ((insn & 0x1FE00000) == 0x1A800000) {
    return ExecuteCondSelect(interpreter, insn);
  }
  if ((insn & 0x7FFF0000) == 0x5AC00000) {
    return ExecuteDataProc1(interpreter, insn);
  }
  if ((insn & 0x7FE00000) == 0x1AC00000) {
    return ExecuteDataProc2(interpreter, insn);
  }
  if ((insn & 0x7F000000) == 0x1B000000) {
    return ExecuteDataProc3(interpreter, insn);
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
  if ((insn & 0xFF000010) == 0x54000000) {
    return ExecuteCondBranch(interpreter, insn, pc_updated);
  }
  if ((insn & 0x3FFFFC00) == 0x085F7C00 ||
      (insn & 0x3FFFFC00) == 0x085FFC00 ||
      (insn & 0x3FE0FC00) == 0x08007C00 ||
      (insn & 0x3FE0FC00) == 0x0800FC00) {
    return ExecuteLoadStoreExclusive(interpreter, insn);
  }
  if ((insn & 0x3FFFFC00) == 0x08DFFC00 ||
      (insn & 0x3FFFFC00) == 0x089FFC00) {
    return ExecuteAcquireRelease(interpreter, insn);
  }
  if ((insn & 0xFFFFF0FF) == 0xD50330BF) {
    return ExecuteBarrier(interpreter, insn);
  }
  if ((insn & 0xFFFFF0FF) == 0xD503305F) {
    interpreter->reservation_valid = false;
    return true;
  }
  if ((insn & 0x3B000000) == 0x39000000 ||
      (insn & 0x3B000000) == 0x38000000) {
    return ExecuteLoadStoreImm(interpreter, insn);
  }
  if ((insn & 0x3E000000) == 0x28000000) {
    return ExecuteLoadStorePair(interpreter, insn);
  }
  if ((insn & 0x5F000000) == 0x1E000000) {
    return ExecuteFP(interpreter, insn);
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

void AARCH64InterpreterWriteX(AARCH64Interpreter* interpreter, int reg,
                              uint64_t value) {
  WriteX(interpreter, reg, value);
}

static uint64_t SetupGuestMainArgs(AARCH64Interpreter* interpreter, int argc,
                                   char** argv, uint64_t entry_address,
                                   bool is_static_link) {
  uint64_t stack_base = (uint64_t)(uintptr_t)interpreter->stack;
  uint64_t stack_top = (stack_base + AARCH64_STACK_SIZE) & ~0xFULL;
  (void)entry_address;
  (void)is_static_link;
  WriteX(interpreter, 0, (uint64_t)argc);
  if (argc <= 0 || argv == NULL) {
    WriteX(interpreter, 1, 0);
    return stack_top;
  }

  size_t string_bytes = 0;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    if (len > AARCH64_STACK_SIZE - string_bytes) {
      goto invalid_args;
    }
    string_bytes += len;
  }
  if ((size_t)argc >= AARCH64_STACK_SIZE / sizeof(uint64_t)) {
    goto invalid_args;
  }
  size_t vector_bytes = (size_t)(argc + 1) * sizeof(uint64_t);
  if (string_bytes + vector_bytes + 15 > AARCH64_STACK_SIZE) {
    goto invalid_args;
  }

  uint64_t string_address = stack_top - string_bytes;
  uint64_t guest_argv = (string_address - vector_bytes) & ~0xFULL;
  char* string_out = (char*)(uintptr_t)string_address;
  uint64_t* pointer_out = (uint64_t*)(uintptr_t)guest_argv;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    memcpy(string_out, argv[i], len);
    pointer_out[i] = (uint64_t)(uintptr_t)string_out;
    string_out += len;
  }
  pointer_out[argc] = 0;
  WriteX(interpreter, 1, guest_argv);
  return guest_argv;

invalid_args:
  WriteX(interpreter, 0, 0);
  WriteX(interpreter, 1, 0);
  return stack_top;
}

void AARCH64InterpreterInitForThread(
    AARCH64Interpreter* interpreter, AARCH64ProcessRuntime* process,
    AARCH64GuestThread* guest_thread, Loader* loader, uint64_t entry_address,
    int argc, char** argv, char* stack, uint64_t tp_base, size_t tls_block_size,
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
    interpreter->stack = malloc(AARCH64_STACK_SIZE);
    interpreter->owns_stack = true;
  }
  if (tp_base != 0) {
    interpreter->tp_base = tp_base;
    interpreter->tls_block_size = tls_block_size;
  } else if (loader->tls.present) {
    interpreter->tp_base = loader->tls.tp_base;
    interpreter->tls_block_size = loader->tls.block_size;
  }
  uint64_t argument_bottom =
      SetupGuestMainArgs(interpreter, argc, argv, entry_address,
                         loader->is_static);
  interpreter->sp = argument_bottom & ~0xFULL;
  interpreter->pc = entry_address;
  interpreter->running = entry_address != 0;
  WriteX(interpreter, AARCH64_LR_REG, 0);
}

void AARCH64InterpreterInit(AARCH64Interpreter* interpreter, Loader* loader,
                            AARCH64ProcessRuntime* process,
                            uint64_t entry_address, int argc, char** argv,
                            bool trace_registers, bool trace_instructions) {
  AARCH64InterpreterInitForThread(interpreter, process, NULL, loader,
                                    entry_address, argc, argv, NULL, 0, 0,
                                    trace_registers, trace_instructions);
}

void AARCH64InterpreterPrepareMain(AARCH64Interpreter* interpreter,
                                   uint64_t entry_address, int argc,
                                   char** argv, bool is_static_link) {
  uint64_t argument_bottom =
      SetupGuestMainArgs(interpreter, argc, argv, entry_address,
                         is_static_link);
  interpreter->sp = argument_bottom & ~0xFULL;
  interpreter->pc = entry_address;
  interpreter->running = true;
  WriteX(interpreter, AARCH64_LR_REG, 0);
}

void AARCH64InterpreterPrepareCall(AARCH64Interpreter* interpreter, uint64_t fn,
                                   uint64_t arg) {
  interpreter->sp =
      (uint64_t)(uintptr_t)(interpreter->stack + AARCH64_STACK_SIZE);
  interpreter->sp &= ~0xFULL;
  interpreter->pc = fn;
  interpreter->running = true;
  WriteX(interpreter, 0, arg);
  WriteX(interpreter, AARCH64_LR_REG, 0);
}

int AARCH64InterpreterCall(AARCH64Interpreter* interpreter, uint64_t fn,
                           uint64_t arg) {
  AARCH64InterpreterPrepareCall(interpreter, fn, arg);
  return AARCH64InterpreterRun(interpreter);
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
      AARCH64InterpreterFail(interpreter, 1);
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
  if (interpreter->owns_stack) {
    free(interpreter->stack);
  }
  interpreter->stack = NULL;
}
