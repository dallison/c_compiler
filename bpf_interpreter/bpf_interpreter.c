#include "bpf_interpreter.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "davecc_guest_syscalls.h"
#include "bpf_machine.h"
#include "elf.h"
#include "posix_host.h"

// call imm used by libc's syscall().  Not a kernel helper.
#define BPF_HOST_SYSCALL 2147483647

void BPFInterpreterInit(BPFInterpreter* interp) {
  memset(interp, 0, sizeof(*interp));
  interp->stack = calloc(1, BPF_INTERP_STACK);
}

void BPFInterpreterDestruct(BPFInterpreter* interp) {
  free(interp->stack);
  for (int i = 0; i < interp->depth; i++) {
    free(interp->frames[i].stack);
  }
}

static uint64_t ReadGuest(uint64_t addr, int size) {
  uint64_t value = 0;
  memcpy(&value, (const void*)(uintptr_t)addr, (size_t)size);
  return value;
}

static void WriteGuest(uint64_t addr, uint64_t value, int size) {
  memcpy((void*)(uintptr_t)addr, &value, (size_t)size);
}

static BpfInsn Fetch(uint64_t pc) {
  uint64_t word = 0;
  memcpy(&word, (const void*)(uintptr_t)pc, sizeof(word));
  return BpfDecodeInsn(word);
}

static int SizeFromCode(uint8_t code) {
  switch (BPF_SIZE(code)) {
    case BPF_B:
      return 1;
    case BPF_H:
      return 2;
    case BPF_W:
      return 4;
    default:
      return 8;
  }
}

static uint64_t Alu(uint8_t op, uint64_t dst, uint64_t src, bool is64) {
  uint32_t d32 = (uint32_t)dst;
  uint32_t s32 = (uint32_t)src;
  uint64_t result;
  switch (op) {
    case BPF_ADD:
      result = is64 ? dst + src : (uint64_t)(d32 + s32);
      break;
    case BPF_SUB:
      result = is64 ? dst - src : (uint64_t)(d32 - s32);
      break;
    case BPF_MUL:
      result = is64 ? dst * src : (uint64_t)(d32 * s32);
      break;
    case BPF_DIV:
      if (is64) {
        result = src == 0 ? 0 : dst / src;
      } else {
        result = s32 == 0 ? 0 : (uint64_t)(d32 / s32);
      }
      break;
    case BPF_OR:
      result = is64 ? dst | src : (uint64_t)(d32 | s32);
      break;
    case BPF_AND:
      result = is64 ? dst & src : (uint64_t)(d32 & s32);
      break;
    case BPF_LSH:
      result = is64 ? dst << (src & 63) : (uint64_t)(d32 << (s32 & 31));
      break;
    case BPF_RSH:
      result = is64 ? dst >> (src & 63) : (uint64_t)(d32 >> (s32 & 31));
      break;
    case BPF_NEG:
      result = is64 ? (uint64_t)(-(int64_t)dst) : (uint64_t)(-(int32_t)d32);
      break;
    case BPF_MOD:
      if (is64) {
        result = src == 0 ? dst : dst % src;
      } else {
        result = s32 == 0 ? d32 : (uint64_t)(d32 % s32);
      }
      break;
    case BPF_XOR:
      result = is64 ? dst ^ src : (uint64_t)(d32 ^ s32);
      break;
    case BPF_MOV:
      result = is64 ? src : (uint64_t)s32;
      break;
    case BPF_ARSH:
      if (is64) {
        result = (uint64_t)((int64_t)dst >> (src & 63));
      } else {
        result = (uint64_t)((int32_t)d32 >> (s32 & 31));
      }
      break;
    default:
      fprintf(stderr, "unsupported ALU op 0x%x\n", op);
      result = dst;
      break;
  }
  if (!is64) {
    result = (uint32_t)result;
  }
  return result;
}

static void* BpfGuestPtr(void* context, uint64_t guest, size_t size) {
  (void)context;
  if (guest == 0) {
    return NULL;
  }
  void* mapped = DaveHostMappingFind(guest, size == 0 ? 1 : size);
  if (mapped != NULL) {
    return mapped;
  }
  return (void*)(uintptr_t)guest;
}

static int64_t BpfHostSyscall(BPFInterpreter* interp) {
  int64_t number = (int64_t)interp->r[1];
  uint64_t a0 = interp->r[2];
  uint64_t a1 = interp->r[3];
  uint64_t a2 = interp->r[4];
  uint64_t a3 = interp->r[5];
  if (number >= DAVE_SYS_MMAP && number <= DAVE_SYS_UNAME) {
    return DaveHostPosixSyscall((int)number, 8, 64, BpfGuestPtr, interp, a0, a1,
                                a2, a3, 0, 0);
  }
  switch (number) {
    case DAVE_SYS_OPEN:
      return open((const char*)(uintptr_t)a0, (int)a1, (mode_t)a2);
    case DAVE_SYS_CLOSE:
      return close((int)a0);
    case DAVE_SYS_WRITE:
      return write((int)a0, (const void*)(uintptr_t)a1, (size_t)a2);
    case DAVE_SYS_READ:
      return read((int)a0, (void*)(uintptr_t)a1, (size_t)a2);
    case DAVE_SYS_LSEEK:
      return lseek((int)a0, (off_t)a1, (int)a2);
    case DAVE_SYS_ABORT:
      interp->exit_code = 134;
      interp->running = false;
      return 0;
    case DAVE_SYS_EXIT:
    case DAVE_SYS_EXIT_CLEAN:
      interp->exit_code = (int)a0;
      interp->running = false;
      return 0;
    case DAVE_SYS_TIME:
      return (int64_t)time(NULL);
    case DAVE_SYS_CLOCK:
    case DAVE_SYS_MONOTONIC_TIME:
    case DAVE_SYS_REALTIME_TIME: {
      struct timespec now;
      clockid_t clock_id =
          number == DAVE_SYS_REALTIME_TIME ? CLOCK_REALTIME : CLOCK_MONOTONIC;
      if (clock_gettime(clock_id, &now) != 0) {
        return -1;
      }
      return (int64_t)now.tv_sec * 1000000000ll + now.tv_nsec;
    }
    default:
      fprintf(stderr, "unsupported eBPF syscall %" PRId64 "\n", number);
      return -ENOSYS;
  }
}

static bool JumpTaken(uint8_t op, uint64_t dst, uint64_t src) {
  switch (op) {
    case BPF_JA:
      return true;
    case BPF_JEQ:
      return dst == src;
    case BPF_JGT:
      return dst > src;
    case BPF_JGE:
      return dst >= src;
    case BPF_JSET:
      return (dst & src) != 0;
    case BPF_JNE:
      return dst != src;
    case BPF_JSGT:
      return (int64_t)dst > (int64_t)src;
    case BPF_JSGE:
      return (int64_t)dst >= (int64_t)src;
    case BPF_JLT:
      return dst < src;
    case BPF_JLE:
      return dst <= src;
    case BPF_JSLT:
      return (int64_t)dst < (int64_t)src;
    case BPF_JSLE:
      return (int64_t)dst <= (int64_t)src;
    default:
      return false;
  }
}

static bool PushFrame(BPFInterpreter* interp, uint64_t ret_pc) {
  if (interp->depth >= BPF_INTERP_MAX_FRAMES) {
    fprintf(stderr, "eBPF call stack overflow\n");
    return false;
  }
  BPFCallFrame* frame = &interp->frames[interp->depth++];
  frame->ret_pc = ret_pc;
  frame->r6 = interp->r[6];
  frame->r7 = interp->r[7];
  frame->r8 = interp->r[8];
  frame->r9 = interp->r[9];
  frame->fp = interp->r[10];
  frame->stack = calloc(1, BPF_INTERP_STACK);
  if (frame->stack == NULL) {
    return false;
  }
  interp->r[10] = (uint64_t)(uintptr_t)(frame->stack + BPF_INTERP_STACK);
  return true;
}

static bool PopFrame(BPFInterpreter* interp) {
  if (interp->depth <= 0) {
    return false;
  }
  BPFCallFrame* frame = &interp->frames[--interp->depth];
  interp->pc = frame->ret_pc;
  interp->r[6] = frame->r6;
  interp->r[7] = frame->r7;
  interp->r[8] = frame->r8;
  interp->r[9] = frame->r9;
  interp->r[10] = frame->fp;
  free(frame->stack);
  frame->stack = NULL;
  return true;
}

int BPFInterpreterRun(BPFInterpreter* interp, Loader* loader, uint64_t entry) {
  interp->loader = loader;
  memset(interp->r, 0, sizeof(interp->r));
  interp->pc = entry;
  interp->r[10] = (uint64_t)(uintptr_t)(interp->stack + BPF_INTERP_STACK);
  interp->running = true;
  interp->exit_code = 0;
  interp->depth = 0;

  uint64_t steps = 0;
  const uint64_t kMaxSteps = 100000000;
  while (interp->running) {
    if (++steps > kMaxSteps) {
      fprintf(stderr, "eBPF interpreter exceeded step limit at 0x%" PRIx64 "\n",
              interp->pc);
      return 1;
    }
    BpfInsn insn = Fetch(interp->pc);
    uint8_t class = BPF_CLASS(insn.code);
    if (interp->trace) {
      fprintf(stderr, "pc=%" PRIx64 " code=%02x dst=r%d src=r%d off=%d imm=%d\n",
              interp->pc, insn.code, insn.dst_reg, insn.src_reg, insn.off,
              insn.imm);
    }
    if (class == BPF_ALU || class == BPF_ALU64) {
      bool is64 = class == BPF_ALU64;
      uint64_t src = (insn.code & BPF_X) ? interp->r[insn.src_reg]
                                         : (uint64_t)(int64_t)insn.imm;
      uint8_t op = BPF_ALU_OP(insn.code);
      interp->r[insn.dst_reg] = Alu(op, interp->r[insn.dst_reg], src, is64);
      interp->pc += BPF_INSN_SIZE;
      continue;
    }
    if (class == BPF_LDX) {
      int size = SizeFromCode(insn.code);
      uint64_t addr = interp->r[insn.src_reg] + (int64_t)insn.off;
      interp->r[insn.dst_reg] = ReadGuest(addr, size);
      interp->pc += BPF_INSN_SIZE;
      continue;
    }
    if (class == BPF_STX) {
      int size = SizeFromCode(insn.code);
      uint64_t addr = interp->r[insn.dst_reg] + (int64_t)insn.off;
      WriteGuest(addr, interp->r[insn.src_reg], size);
      interp->pc += BPF_INSN_SIZE;
      continue;
    }
    if (class == BPF_ST) {
      int size = SizeFromCode(insn.code);
      uint64_t addr = interp->r[insn.dst_reg] + (int64_t)insn.off;
      WriteGuest(addr, (uint64_t)(int64_t)insn.imm, size);
      interp->pc += BPF_INSN_SIZE;
      continue;
    }
    if (class == BPF_LD) {
      if (BPF_SIZE(insn.code) == BPF_DW && BPF_MODE(insn.code) == BPF_IMM) {
        BpfInsn next = Fetch(interp->pc + BPF_INSN_SIZE);
        uint64_t value = (uint64_t)(uint32_t)insn.imm |
                         ((uint64_t)(uint32_t)next.imm << 32);
        interp->r[insn.dst_reg] = value;
        interp->pc += BPF_LDDW_SIZE;
        continue;
      }
      fprintf(stderr, "unsupported BPF_LD encoding 0x%x\n", insn.code);
      return 1;
    }
    if (class == BPF_JMP || class == BPF_JMP32) {
      uint8_t op = BPF_ALU_OP(insn.code);
      if (op == BPF_CALL) {
        uint64_t next = interp->pc + BPF_INSN_SIZE;
        if ((insn.code & BPF_X) == 0 && insn.src_reg != BPF_PSEUDO_CALL &&
            insn.imm == BPF_HOST_SYSCALL) {
          interp->r[0] = (uint64_t)BpfHostSyscall(interp);
          interp->pc = next;
          continue;
        }
        uint64_t target;
        if (insn.code & BPF_X) {
          target = interp->r[insn.dst_reg];
        } else if (insn.src_reg == BPF_PSEUDO_CALL) {
          target = next + (int64_t)insn.imm * BPF_INSN_SIZE;
        } else {
          fprintf(stderr, "unsupported eBPF helper call %d\n", insn.imm);
          return 1;
        }
        if (target == 0 || (target % BPF_INSN_SIZE) != 0) {
          fprintf(stderr, "invalid eBPF call target 0x%" PRIx64 "\n", target);
          return 1;
        }
        if (!PushFrame(interp, next)) {
          return 1;
        }
        interp->pc = target;
        continue;
      }
      // gotol: BPF_JMP32 | BPF_JA carries a 32-bit instruction delta in imm.
      if (class == BPF_JMP32 && op == BPF_JA) {
        interp->pc += BPF_INSN_SIZE + (int64_t)insn.imm * BPF_INSN_SIZE;
        continue;
      }
      if (op == BPF_EXIT) {
        if (interp->depth == 0) {
          interp->running = false;
          interp->exit_code = (int)(uint32_t)interp->r[0];
          break;
        }
        uint64_t rv = interp->r[0];
        if (!PopFrame(interp)) {
          return 1;
        }
        interp->r[0] = rv;
        continue;
      }
      uint64_t src = (insn.code & BPF_X) ? interp->r[insn.src_reg]
                                         : (uint64_t)(int64_t)insn.imm;
      uint64_t dst = interp->r[insn.dst_reg];
      if (class == BPF_JMP32) {
        src = (uint32_t)src;
        dst = (uint32_t)dst;
      }
      if (JumpTaken(op, dst, src)) {
        interp->pc += BPF_INSN_SIZE + (int64_t)insn.off * BPF_INSN_SIZE;
      } else {
        interp->pc += BPF_INSN_SIZE;
      }
      continue;
    }
    fprintf(stderr, "unsupported eBPF opcode 0x%x at 0x%" PRIx64 "\n",
            insn.code, interp->pc);
    return 1;
  }
  return interp->exit_code;
}
