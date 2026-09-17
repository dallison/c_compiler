#include "disassembler_internal.h"

#include "bpf_machine.h"

#include <inttypes.h>
#include <stdio.h>

static const char* AluName(uint8_t op, bool is64) {
  switch (op) {
    case BPF_ADD:
      return is64 ? "add64" : "add32";
    case BPF_SUB:
      return is64 ? "sub64" : "sub32";
    case BPF_MUL:
      return is64 ? "mul64" : "mul32";
    case BPF_DIV:
      return is64 ? "div64" : "div32";
    case BPF_OR:
      return is64 ? "or64" : "or32";
    case BPF_AND:
      return is64 ? "and64" : "and32";
    case BPF_LSH:
      return is64 ? "lsh64" : "lsh32";
    case BPF_RSH:
      return is64 ? "rsh64" : "rsh32";
    case BPF_NEG:
      return is64 ? "neg64" : "neg32";
    case BPF_MOD:
      return is64 ? "mod64" : "mod32";
    case BPF_XOR:
      return is64 ? "xor64" : "xor32";
    case BPF_MOV:
      return is64 ? "mov64" : "mov32";
    case BPF_ARSH:
      return is64 ? "arsh64" : "arsh32";
    default:
      return is64 ? "alu64" : "alu32";
  }
}

static const char* JccName(uint8_t op) {
  switch (op) {
    case BPF_JA:
      return "ja";
    case BPF_JEQ:
      return "jeq";
    case BPF_JGT:
      return "jgt";
    case BPF_JGE:
      return "jge";
    case BPF_JSET:
      return "jset";
    case BPF_JNE:
      return "jne";
    case BPF_JSGT:
      return "jsgt";
    case BPF_JSGE:
      return "jsge";
    case BPF_JLT:
      return "jlt";
    case BPF_JLE:
      return "jle";
    case BPF_JSLT:
      return "jslt";
    case BPF_JSLE:
      return "jsle";
    default:
      return "jcc";
  }
}

static const char* MemSize(uint8_t code) {
  switch (BPF_SIZE(code)) {
    case BPF_B:
      return "b";
    case BPF_H:
      return "h";
    case BPF_W:
      return "w";
    default:
      return "dw";
  }
}

bool DAsmDisassembleBPF(const void* bytes, size_t length, uint64_t address,
                        DAsmInstruction* inst) {
  if (length < BPF_INSN_SIZE) {
    return false;
  }
  const unsigned char* p = bytes;
  uint64_t word = DAsmRead64LE(p);
  BpfInsn insn = BpfDecodeInsn(word);
  uint8_t class = BPF_CLASS(insn.code);
  if (class == BPF_LD && BPF_SIZE(insn.code) == BPF_DW &&
      BPF_MODE(insn.code) == BPF_IMM) {
    if (length < BPF_LDDW_SIZE) {
      return false;
    }
    BpfInsn next = BpfDecodeInsn(DAsmRead64LE(p + BPF_INSN_SIZE));
    uint64_t value = (uint64_t)(uint32_t)insn.imm |
                     ((uint64_t)(uint32_t)next.imm << 32);
    DAsmInitInstruction(inst, bytes, length, address, BPF_LDDW_SIZE);
    DAsmFormat(inst, "lddw r%d, 0x%" PRIx64, insn.dst_reg, value);
    return true;
  }

  DAsmInitInstruction(inst, bytes, length, address, BPF_INSN_SIZE);
  if (class == BPF_ALU || class == BPF_ALU64) {
    bool is64 = class == BPF_ALU64;
    uint8_t op = BPF_ALU_OP(insn.code);
    if (op == BPF_NEG) {
      DAsmFormat(inst, "%s r%d", AluName(op, is64), insn.dst_reg);
    } else if (insn.code & BPF_X) {
      DAsmFormat(inst, "%s r%d, r%d", AluName(op, is64), insn.dst_reg,
                 insn.src_reg);
    } else {
      DAsmFormat(inst, "%s r%d, %d", AluName(op, is64), insn.dst_reg, insn.imm);
    }
    return true;
  }
  if (class == BPF_LDX) {
    DAsmFormat(inst, "ldx%s r%d, [r%d%+d]", MemSize(insn.code), insn.dst_reg,
               insn.src_reg, insn.off);
    return true;
  }
  if (class == BPF_STX) {
    DAsmFormat(inst, "stx%s [r%d%+d], r%d", MemSize(insn.code), insn.dst_reg,
               insn.off, insn.src_reg);
    return true;
  }
  if (class == BPF_ST) {
    DAsmFormat(inst, "st%s [r%d%+d], %d", MemSize(insn.code), insn.dst_reg,
               insn.off, insn.imm);
    return true;
  }
  if (class == BPF_JMP || class == BPF_JMP32) {
    uint8_t op = BPF_ALU_OP(insn.code);
    if (op == BPF_CALL) {
      if (insn.code & BPF_X) {
        DAsmFormat(inst, "callx r%d", insn.dst_reg);
      } else {
        DAsmFormat(inst, "call %d", insn.imm);
      }
      return true;
    }
    if (op == BPF_EXIT) {
      DAsmFormat(inst, "exit");
      return true;
    }
    uint64_t target =
        address + BPF_INSN_SIZE + (int64_t)insn.off * BPF_INSN_SIZE;
    DAsmSetTarget(inst, target);
    if (op == BPF_JA) {
      DAsmFormat(inst, "ja 0x%" PRIx64, target);
      return true;
    }
    if (insn.code & BPF_X) {
      DAsmFormat(inst, "%s r%d, r%d, 0x%" PRIx64, JccName(op), insn.dst_reg,
                 insn.src_reg, target);
    } else {
      DAsmFormat(inst, "%s r%d, %d, 0x%" PRIx64, JccName(op), insn.dst_reg,
                 insn.imm, target);
    }
    return true;
  }
  DAsmUnknownInstruction(inst, ".qword 0x%" PRIx64, word);
  return true;
}
