#include "disassembler_internal.h"

#include "xtensa_isa.h"

static uint64_t BranchTarget(uint64_t address,
                             const XtensaInstruction* instruction) {
  return address + 4 + instruction->immediate;
}

bool DAsmDisassembleXtensa(const void* bytes, size_t length, uint64_t address,
                           DAsmInstruction* output) {
  const unsigned char* data = bytes;
  if (data == NULL || length == 0) {
    return false;
  }
  size_t size = length >= 2 && (data[0] & 0xf) >= 8 ? 2 : 3;
  DAsmInitInstruction(output, bytes, length, address, size);

  XtensaInstruction instruction;
  if (!XtensaDecode(bytes, length, &instruction)) {
    DAsmUnknownInstruction(output, ".byte 0x%02llx", data[0]);
    return false;
  }

  const char* name = XtensaInstructionName(instruction.kind);
  switch (instruction.kind) {
    case kXtensaAdd:
    case kXtensaSub:
    case kXtensaAnd:
    case kXtensaOr:
    case kXtensaXor:
    case kXtensaMull:
    case kXtensaQuou:
    case kXtensaQuos:
    case kXtensaRemu:
    case kXtensaRems:
      DAsmFormat(output, "%s a%u, a%u, a%u", name, instruction.rd,
                 instruction.rs, instruction.rt);
      break;
    case kXtensaNeg:
    case kXtensaSll:
    case kXtensaSrl:
    case kXtensaSra:
      DAsmFormat(output, "%s a%u, a%u", name, instruction.rd,
                 instruction.rs);
      break;
    case kXtensaAddi:
    case kXtensaSlli:
    case kXtensaSrai:
    case kXtensaSrli:
      DAsmFormat(output, "%s a%u, a%u, %d", name, instruction.rd,
                 instruction.rs, instruction.immediate);
      break;
    case kXtensaMovi:
      DAsmFormat(output, "%s a%u, %d", name, instruction.rd,
                 instruction.immediate);
      break;
    case kXtensaL8ui:
    case kXtensaL16ui:
    case kXtensaL16si:
    case kXtensaL32i:
    case kXtensaS8i:
    case kXtensaS16i:
    case kXtensaS32i:
      DAsmFormat(output, "%s a%u, a%u, %d", name, instruction.rd,
                 instruction.rs, instruction.immediate);
      break;
    case kXtensaBeq:
    case kXtensaBne:
    case kXtensaBlt:
    case kXtensaBge:
    case kXtensaBltu:
    case kXtensaBgeu:
      DAsmSetTarget(output, BranchTarget(address, &instruction));
      DAsmFormat(output, "%s a%u, a%u, 0x%llx", name, instruction.rs,
                 instruction.rt,
                 (unsigned long long)output->target_address);
      break;
    case kXtensaBeqz:
    case kXtensaBnez:
    case kXtensaBltz:
    case kXtensaBgez:
      DAsmSetTarget(output, BranchTarget(address, &instruction));
      DAsmFormat(output, "%s a%u, 0x%llx", name, instruction.rs,
                 (unsigned long long)output->target_address);
      break;
    case kXtensaL32r:
      DAsmSetTarget(output, ((address + 3) & ~3ull) + instruction.immediate);
      DAsmFormat(output, "%s a%u, 0x%llx", name, instruction.rd,
                 (unsigned long long)output->target_address);
      break;
    case kXtensaJ:
      DAsmSetTarget(output, BranchTarget(address, &instruction));
      DAsmFormat(output, "%s 0x%llx", name,
                 (unsigned long long)output->target_address);
      break;
    case kXtensaCall8:
      DAsmSetTarget(output,
                    (address & ~3ull) + 4 + instruction.immediate);
      DAsmFormat(output, "%s 0x%llx", name,
                 (unsigned long long)output->target_address);
      break;
    case kXtensaJx:
    case kXtensaCallx8:
    case kXtensaSsl:
    case kXtensaSsr:
      DAsmFormat(output, "%s a%u", name, instruction.rs);
      break;
    case kXtensaEntry:
      DAsmFormat(output, "%s a%u, %d", name, instruction.rs,
                 instruction.immediate);
      break;
    case kXtensaBreak:
      DAsmFormat(output, "%s %d", name, instruction.immediate);
      break;
    case kXtensaRetw:
      DAsmFormat(output, "%s", name);
      break;
    case kXtensaInvalid:
      return false;
  }
  return true;
}
