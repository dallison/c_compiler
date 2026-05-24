//
//  arm_interpreter.c
//  arm_interpreter
//

#include "arm_interpreter.h"
#include "elf.h"
#include "loader_dynamic.h"
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define ARM_RESOLVER_LINKED 0x42000000u

bool print_libraries_only = false;

#define CPSR_N (1u << 31)
#define CPSR_Z (1u << 30)
#define CPSR_C (1u << 29)
#define CPSR_V (1u << 28)

static uint64_t ReadReg(ARMInterpreter* interpreter, int reg) {
  if (reg == ARM_PC_REG) {
    return interpreter->pc + 8;
  }
  return interpreter->regs[reg];
}

static void WriteReg(ARMInterpreter* interpreter, int reg, uint64_t value) {
  if (reg == ARM_PC_REG) {
    return;
  }
  interpreter->regs[reg] = value;
}

static uint64_t RuntimeToLinked(ARMInterpreter* interpreter, uint64_t runtime) {
  uint64_t linked = runtime & 0xffffffffu;
  if (LoaderRuntimeAddressToLinked(interpreter->loader, runtime, &linked)) {
    return linked;
  }
  return runtime & 0xffffffffu;
}

static uint64_t LinkedToRuntime(ARMInterpreter* interpreter, uint32_t linked) {
  uint64_t runtime = linked;
  LoaderLinkedAddressToRuntime(interpreter->loader, NULL, linked, &runtime);
  return runtime;
}

static int64_t SegmentFileOffsetDelta(const ELFProgramHeader* segment) {
  int64_t page_size = sysconf(_SC_PAGESIZE);
  return segment->offset - (segment->offset & ~(page_size - 1));
}

static bool GuestAddressOk(Loader* loader, uint64_t addr, size_t size) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      uint64_t start = (uint64_t)(uintptr_t)region->address;
      uint64_t end = start + (uint64_t)region->length;
      if (addr >= start && addr + size <= end) {
        return true;
      }
      continue;
    }
    ELFProgramHeader* segment = region->segment;
    uint64_t base = (uint64_t)(uintptr_t)region->address +
                    (uint64_t)SegmentFileOffsetDelta(segment);
    if (addr >= base && addr + size <= base + (uint64_t)region->length) {
      return true;
    }
  }
  return false;
}

static bool InterpreterAddressOk(ARMInterpreter* interpreter, uint64_t addr,
                                 size_t size) {
  if (interpreter->stack != NULL) {
    uint64_t start = (uint64_t)(uintptr_t)interpreter->stack;
    uint64_t end = start + ARM_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return GuestAddressOk(interpreter->loader, addr, size);
}

static uint32_t Fetch32(ARMInterpreter* interpreter) {
  if (!InterpreterAddressOk(interpreter, interpreter->pc, 4)) {
    fprintf(stderr, "Fetch32 outside mapped memory at pc 0x%016" PRIx64 "\n",
            interpreter->pc);
    exit(1);
  }
  return *(uint32_t*)(uintptr_t)interpreter->pc;
}

static void Store8(ARMInterpreter* interpreter, uint64_t addr, uint8_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Store8 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  *(uint8_t*)(uintptr_t)addr = value;
}

static void Store16(ARMInterpreter* interpreter, uint64_t addr, uint16_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Store16 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  *(uint16_t*)(uintptr_t)addr = value;
}

static void Store32(ARMInterpreter* interpreter, uint64_t addr, uint32_t value) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  *(uint32_t*)(uintptr_t)addr = value;
}

static uint8_t Load8(ARMInterpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 1)) {
    fprintf(stderr, "Load8 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  return *(uint8_t*)(uintptr_t)addr;
}

static uint16_t Load16(ARMInterpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 2)) {
    fprintf(stderr, "Load16 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  return *(uint16_t*)(uintptr_t)addr;
}

static uint32_t Load32(ARMInterpreter* interpreter, uint64_t addr) {
  if (!InterpreterAddressOk(interpreter, addr, 4)) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%08x\n", addr);
    exit(1);
  }
  return *(uint32_t*)(uintptr_t)addr;
}

static int32_t SignExtend32(uint32_t value, int bits) {
  uint32_t sign_bit = 1u << (bits - 1);
  return (int32_t)((value ^ sign_bit) - sign_bit);
}

static uint32_t RotateRight32(uint32_t value, int amount) {
  amount &= 31;
  if (amount == 0) {
    return value;
  }
  return (value >> amount) | (value << (32 - amount));
}

static uint32_t DecodeImmRotate(uint32_t encoded) {
  uint32_t imm = encoded & 0xffu;
  uint32_t rot = (encoded >> 7) & 0x1eu;
  return RotateRight32(imm, rot);
}

static void SetNZ(ARMInterpreter* interpreter, uint32_t result) {
  interpreter->cpsr &= ~(CPSR_N | CPSR_Z);
  if (result == 0) {
    interpreter->cpsr |= CPSR_Z;
  }
  if (result & 0x80000000u) {
    interpreter->cpsr |= CPSR_N;
  }
}

static void SetNZCVAdd(ARMInterpreter* interpreter, uint32_t lhs, uint32_t rhs,
                       uint32_t result) {
  SetNZ(interpreter, result);
  interpreter->cpsr &= ~(CPSR_C | CPSR_V);
  if ((result < lhs) || (rhs > 0 && result < rhs)) {
    interpreter->cpsr |= CPSR_C;
  }
  if (((lhs ^ result) & (rhs ^ result) & 0x80000000u) != 0) {
    interpreter->cpsr |= CPSR_V;
  }
}

static void SetNZCVSub(ARMInterpreter* interpreter, uint32_t lhs, uint32_t rhs,
                       uint32_t result) {
  SetNZ(interpreter, result);
  interpreter->cpsr &= ~(CPSR_C | CPSR_V);
  if (lhs >= rhs) {
    interpreter->cpsr |= CPSR_C;
  }
  if (((lhs ^ rhs) & (lhs ^ result) & 0x80000000u) != 0) {
    interpreter->cpsr |= CPSR_V;
  }
}

static bool ConditionPass(ARMInterpreter* interpreter, uint32_t cond) {
  bool n = (interpreter->cpsr & CPSR_N) != 0;
  bool z = (interpreter->cpsr & CPSR_Z) != 0;
  bool c = (interpreter->cpsr & CPSR_C) != 0;
  bool v = (interpreter->cpsr & CPSR_V) != 0;
  switch (cond) {
    case ARM_COND_EQ:
      return z;
    case ARM_COND_NE:
      return !z;
    case ARM_COND_CS:
      return c;
    case ARM_COND_CC:
      return !c;
    case ARM_COND_MI:
      return n;
    case ARM_COND_PL:
      return !n;
    case ARM_COND_VS:
      return v;
    case ARM_COND_VC:
      return !v;
    case ARM_COND_HI:
      return c && !z;
    case ARM_COND_LS:
      return !c || z;
    case ARM_COND_GE:
      return n == v;
    case ARM_COND_LT:
      return n != v;
    case ARM_COND_GT:
      return !z && (n == v);
    case ARM_COND_LE:
      return z || (n != v);
    case ARM_COND_AL:
    default:
      return true;
  }
}

static bool ShiftOperand(ARMInterpreter* interpreter, uint32_t value,
                           int shift_type, int shift_amount, bool carry_in,
                           uint32_t* out, bool update_carry) {
  if (shift_amount == 0 && shift_type == 3) {
    *out = carry_in ? 1u : 0u;
    return true;
  }
  if (shift_amount >= 32) {
    if (shift_type == 0 || shift_type == 3) {
      *out = 0;
      if (update_carry) {
        interpreter->cpsr &= ~CPSR_C;
        if (shift_amount > 0 && shift_type == 0 && (value & (1u << (32 - (shift_amount % 32)))) != 0) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      return true;
    }
    if (shift_type == 1) {
      *out = 0;
      if (update_carry && shift_amount == 32) {
        interpreter->cpsr &= ~CPSR_C;
        if (value & 0x80000000u) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      return true;
    }
    if (shift_type == 2) {
      *out = (shift_amount >= 32) ? (int32_t)value >> 31 : (uint32_t)((int32_t)value >> shift_amount);
      if (update_carry && shift_amount == 32) {
        interpreter->cpsr &= ~CPSR_C;
        if (value & 0x80000000u) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      return true;
    }
  }
  switch (shift_type) {
    case 0:
      if (update_carry && shift_amount > 0) {
        interpreter->cpsr &= ~CPSR_C;
        if ((value >> (32 - shift_amount)) & 1u) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      *out = value << shift_amount;
      return true;
    case 1:
      if (update_carry && shift_amount > 0) {
        interpreter->cpsr &= ~CPSR_C;
        if ((value >> (shift_amount - 1)) & 1u) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      *out = value >> shift_amount;
      return true;
    case 2:
      if (update_carry && shift_amount > 0) {
        interpreter->cpsr &= ~CPSR_C;
        if (((int32_t)value >> (shift_amount - 1)) & 1) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      *out = (uint32_t)((int32_t)value >> shift_amount);
      return true;
    case 3:
      if (shift_amount == 0) {
        *out = carry_in ? 1u : 0u;
        return true;
      }
      if (update_carry) {
        interpreter->cpsr &= ~CPSR_C;
        if ((value >> (shift_amount - 1)) & 1u) {
          interpreter->cpsr |= CPSR_C;
        }
      }
      *out = RotateRight32(value, shift_amount);
      return true;
    default:
      return false;
  }
}

static bool DecodeRegShift(ARMInterpreter* interpreter, uint32_t insn,
                           uint32_t* operand, bool update_carry) {
  uint32_t rm = insn & 0xfu;
  uint32_t value = ReadReg(interpreter, (int)rm);
  int shift_type = (int)((insn >> 5) & 3u);
  int shift_amount;
  if ((insn & 0x10u) != 0) {
    shift_amount = (int)(ReadReg(interpreter, (int)((insn >> 8) & 0xfu)) & 0xffu);
  } else {
    shift_amount = (int)((insn >> 7) & 0x1fu);
  }
  bool carry = (interpreter->cpsr & CPSR_C) != 0;
  return ShiftOperand(interpreter, value, shift_type, shift_amount, carry,
                      operand, update_carry);
}

static bool SectionAddressAndSize(LoadedDynamicLibrary* lib, const char* name,
                                  uint64_t* addr, uint64_t* size) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->addr == 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, name) != 0) {
      continue;
    }
    *addr = sh->addr;
    *size = sh->size;
    return true;
  }
  return false;
}

static bool ReadRelaPltEntry(LoadedDynamicLibrary* lib, int64_t index,
                             ELFRelocation* out) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->fd < 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, ".rel.plt") != 0 &&
        strcmp(shstrtab + sh->name, ".rela.plt") != 0) {
      continue;
    }
    off_t file_offset =
        (off_t)(sh->offset + (uint64_t)index * sizeof(ELFRelocation));
    return pread(lib->fd, out, sizeof(*out), file_offset) ==
           (ssize_t)sizeof(*out);
  }
  return false;
}

static uint64_t BranchTarget(ARMInterpreter* interpreter, uint64_t target) {
  if (GuestAddressOk(interpreter->loader, target, 4)) {
    return target;
  }
  if (target > 0xffffffffu) {
    return target;
  }
  return LinkedToRuntime(interpreter, (uint32_t)target);
}

static uint64_t LinkedToRuntimePreferDSO(ARMInterpreter* interpreter,
                                         uint32_t linked) {
  Loader* loader = interpreter->loader;
  uint64_t runtime = linked;

  for (size_t i = 0; i < loader->loaded_libraries.search.length; i++) {
    LoadedDynamicLibrary* lib = loader->loaded_libraries.search.value.p[i];
    if (lib == loader->dynamic_lib) {
      continue;
    }
    uint64_t candidate = linked;
    if (LoaderLinkedAddressToRuntime(loader, lib, linked, &candidate)) {
      return candidate;
    }
  }
  if (LoaderLinkedAddressToRuntime(loader, loader->dynamic_lib, linked, &runtime)) {
    return runtime;
  }
  if (LoaderLinkedAddressToRuntime(loader, NULL, linked, &runtime)) {
    return runtime;
  }
  return linked;
}

static uint64_t BranchTargetPreferDSO(ARMInterpreter* interpreter, uint64_t target) {
  if (GuestAddressOk(interpreter->loader, target, 4)) {
    return target;
  }
  if (target > 0xffffffffu) {
    return target;
  }
  return LinkedToRuntimePreferDSO(interpreter, (uint32_t)target);
}

static bool GotPointsIntoPlt(Loader* loader, LoadedDynamicLibrary* lib,
                             uint64_t got_value) {
  uint64_t plt_linked = 0;
  uint64_t plt_size = 0;
  if (!SectionAddressAndSize(lib, ".plt", &plt_linked, &plt_size)) {
    return false;
  }
  uint64_t plt_runtime = 0;
  if (!LoaderLinkedAddressToRuntime(loader, lib, plt_linked, &plt_runtime)) {
    return false;
  }
  return got_value >= plt_runtime &&
         got_value < plt_runtime + plt_size;
}

static void ResolveAndFixupSymbol(ARMInterpreter* interpreter, bool* pc_updated) {
  Loader* loader = interpreter->loader;
  LoadedDynamicLibrary* lib = loader->dynamic_lib;
  if (lib == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    exit(1);
  }

  int64_t plt_rel_size = 0;
  const DynamicSection* section = lib->dynamic;
  if (section != NULL) {
    for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
      if (section->entries[i].tag == DT(pltrelsz)) {
        plt_rel_size = section->entries[i].un.val;
      }
    }
  }
  int64_t num_relocations = plt_rel_size / (int64_t)sizeof(ELFRelocation);

  const ELFRelocation* reloc = NULL;
  ELFRelocation file_reloc;
  for (int64_t i = 0; i < num_relocations; i++) {
    if (!ReadRelaPltEntry(lib, i, &file_reloc)) {
      continue;
    }
    if (ELF_R_TYPE(file_reloc.info) != R_ARM_JUMP_SLOT) {
      continue;
    }
    uint64_t got_slot = 0;
    if (!LoaderLinkedAddressToRuntime(loader, lib, file_reloc.offset,
                                      &got_slot)) {
      continue;
    }
    uint32_t got_value = *(uint32_t*)(uintptr_t)got_slot;
    if (GotPointsIntoPlt(loader, lib,
                         LinkedToRuntime(interpreter, got_value))) {
      reloc = &file_reloc;
      break;
    }
  }
  if (reloc == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    exit(1);
  }

  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  if (!DynamicLoaderFindSymbol(&loader->loaded_libraries, sym_name, &symbol,
                               &found_lib)) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }

  uint64_t got_runtime = 0;
  if (!LoaderLinkedAddressToRuntime(loader, lib, reloc->offset, &got_runtime)) {
    fprintf(stderr, "Cannot translate GOT slot for %s\n", sym_name);
    exit(1);
  }
  uint64_t runtime = symbol->value;
  if (!LoaderLinkedAddressToRuntime(loader, found_lib, symbol->value, &runtime)) {
    fprintf(stderr, "Cannot translate symbol %s\n", sym_name);
    exit(1);
  }
  *(uint32_t*)(uintptr_t)got_runtime = (uint32_t)symbol->value;
  interpreter->pc = runtime;
  *pc_updated = true;
}

static int32_t HandleSyscall(ARMInterpreter* interpreter, int32_t number,
                             int32_t a0, int32_t a1, int32_t a2, int32_t a3,
                             int32_t a4, int32_t a5, bool* pc_updated) {
  (void)a4;
  (void)a5;
  switch (number) {
    case ARM_SYSCALL_HALT:
    case ARM_SYSCALL_EXIT:
      exit(a0);
      break;
    case ARM_SYSCALL_OPEN:
      return open((const char*)(uintptr_t)a1, a2, (mode_t)a3);
    case ARM_SYSCALL_CLOSE:
      return close(a1);
    case ARM_SYSCALL_READ:
      return (int32_t)read(a1, (void*)(uintptr_t)a2, (size_t)a3);
    case ARM_SYSCALL_WRITE:
      return (int32_t)write(a1, (const void*)(uintptr_t)a2, (size_t)a3);
    case ARM_SYSCALL_LSEEK:
      return (int32_t)lseek(a1, (off_t)a2, a3);
    case ARM_SYSCALL_MALLOC:
      return (int32_t)(uintptr_t)malloc((size_t)a1);
    case ARM_SYSCALL_REALLOC:
      return (int32_t)(uintptr_t)realloc((void*)(uintptr_t)a1, (size_t)a2);
    case ARM_SYSCALL_FREE:
      free((void*)(uintptr_t)a1);
      return 0;
    case ARM_SYSCALL_ABORT:
      abort();
      break;
    case ARM_SYSCALL_RESOLVE:
      ResolveAndFixupSymbol(interpreter, pc_updated);
      return 0;
    default:
      fprintf(stderr, "Unknown ARM syscall %d\n", number);
      ARMInterpreterDumpRegisters(interpreter);
      exit(1);
  }
  return -1;
}

static bool ExecuteDataProcessingImm(ARMInterpreter* interpreter, uint32_t insn) {
  int opcode = (int)((insn >> 21) & 0xfu);
  bool set_flags = ((insn >> 20) & 1u) != 0;
  int rn = (int)((insn >> 16) & 0xfu);
  int rd = (int)((insn >> 12) & 0xfu);
  uint32_t rhs = DecodeImmRotate(insn & 0xfffu);
  uint64_t lhs = ReadReg(interpreter, rn);
  uint64_t result = 0;
  switch (opcode) {
    case 0x0:
      result = lhs & rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0x1:
      result = lhs ^ rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0x2:
      result = lhs - rhs;
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, result);
      }
      break;
    case 0x3:
      result = rhs - lhs;
      if (set_flags) {
        SetNZCVSub(interpreter, rhs, lhs, result);
      }
      break;
    case 0x4:
      result = lhs + rhs;
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, result);
      }
      break;
    case 0x5:
      result = lhs + rhs + ((interpreter->cpsr & CPSR_C) ? 1u : 0u);
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, result);
      }
      break;
    case 0x6:
      result = lhs - rhs - (((interpreter->cpsr & CPSR_C) ? 0u : 1u));
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, result);
      }
      break;
    case 0x8:
      if (set_flags) {
        SetNZ(interpreter, lhs & rhs);
      }
      return true;
    case 0x9:
      if (set_flags) {
        SetNZ(interpreter, lhs ^ rhs);
      }
      return true;
    case 0xa:
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, lhs - rhs);
      }
      return true;
    case 0xb:
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, lhs + rhs);
      }
      return true;
    case 0xc:
      result = lhs | rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xd:
      result = rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xe:
      result = lhs & ~rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xf:
      result = ~rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    default:
      return false;
  }
  WriteReg(interpreter, rd, result);
  return true;
}

static bool ExecuteDataProcessingReg(ARMInterpreter* interpreter, uint32_t insn) {
  int opcode = (int)((insn >> 21) & 0xfu);
  bool set_flags = ((insn >> 20) & 1u) != 0;
  int rn = (int)((insn >> 16) & 0xfu);
  int rd = (int)((insn >> 12) & 0xfu);
  uint64_t lhs = ReadReg(interpreter, rn);
  uint64_t rhs = 0;
  if (!DecodeRegShift(interpreter, insn, &rhs, set_flags)) {
    return false;
  }
  uint64_t result = 0;
  switch (opcode) {
    case 0x0:
      result = lhs & rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0x1:
      result = lhs ^ rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0x2:
      result = lhs - rhs;
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, result);
      }
      break;
    case 0x3:
      result = rhs - lhs;
      if (set_flags) {
        SetNZCVSub(interpreter, rhs, lhs, result);
      }
      break;
    case 0x4:
      result = lhs + rhs;
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, result);
      }
      break;
    case 0x5:
      result = lhs + rhs + ((interpreter->cpsr & CPSR_C) ? 1u : 0u);
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, result);
      }
      break;
    case 0x6:
      result = lhs - rhs - (((interpreter->cpsr & CPSR_C) ? 0u : 1u));
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, result);
      }
      break;
    case 0x8:
      if (set_flags) {
        SetNZ(interpreter, lhs & rhs);
      }
      return true;
    case 0x9:
      if (set_flags) {
        SetNZ(interpreter, lhs ^ rhs);
      }
      return true;
    case 0xa:
      if (set_flags) {
        SetNZCVSub(interpreter, lhs, rhs, lhs - rhs);
      }
      return true;
    case 0xb:
      if (set_flags) {
        SetNZCVAdd(interpreter, lhs, rhs, lhs + rhs);
      }
      return true;
    case 0xc:
      result = lhs | rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xd:
      result = rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xe:
      result = lhs & ~rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    case 0xf:
      result = ~rhs;
      if (set_flags) {
        SetNZ(interpreter, result);
      }
      break;
    default:
      return false;
  }
  WriteReg(interpreter, rd, result);
  return true;
}

static bool ExecuteMultiply(ARMInterpreter* interpreter, uint32_t insn) {
  int rd = (int)((insn >> 16) & 0xfu);
  int rs = (int)((insn >> 8) & 0xfu);
  int rm = (int)(insn & 0xfu);
  uint32_t result = ReadReg(interpreter, rm) * ReadReg(interpreter, rs);
  WriteReg(interpreter, rd, result);
  return true;
}

static bool ExecuteMovwMovt(ARMInterpreter* interpreter, uint32_t insn) {
  bool is_movt = ((insn >> 22) & 1u) != 0;
  int rd = (int)((insn >> 12) & 0xfu);
  uint32_t imm16 = (insn & 0xfffu) | ((insn >> 4) & 0xf000u);
  uint32_t value = ReadReg(interpreter, rd);
  if (is_movt) {
    value = (value & 0xffffu) | (imm16 << 16);
  } else {
    value = (value & 0xffff0000u) | imm16;
  }
  WriteReg(interpreter, rd, value);
  return true;
}

static bool ExecuteLoadStore(ARMInterpreter* interpreter, uint32_t insn,
                             bool* pc_updated) {
  bool preindex = ((insn >> 24) & 1u) != 0;
  bool add_offset = ((insn >> 23) & 1u) != 0;
  bool byte = ((insn >> 22) & 1u) != 0;
  bool writeback = ((insn >> 21) & 1u) != 0;
  bool load = ((insn >> 20) & 1u) != 0;
  int rn = (int)((insn >> 16) & 0xfu);
  int rd = (int)((insn >> 12) & 0xfu);
  bool halfword = !byte && ((insn >> 5) & 1u) != 0;
  uint32_t offset = insn & 0xfffu;
  if ((insn & 0x02000000u) != 0) {
    uint32_t shifted = 0;
    if (!DecodeRegShift(interpreter, insn, &shifted, false)) {
      return false;
    }
    offset = shifted;
  }
  uint64_t base = ReadReg(interpreter, rn);
  uint64_t addr = base;
  if (preindex) {
    addr = add_offset ? base + offset : base - offset;
    if (writeback) {
      WriteReg(interpreter, rn, addr);
    }
  }
  if (load) {
    if (rd == ARM_PC_REG) {
      uint32_t target;
      if (halfword) {
        target = Load16(interpreter, addr);
      } else if (byte) {
        target = Load8(interpreter, addr);
      } else {
        target = Load32(interpreter, addr);
      }
      if (target == 0 && !interpreter->loader->is_static) {
        bool resolve_pc_updated = false;
        ResolveAndFixupSymbol(interpreter, &resolve_pc_updated);
        *pc_updated = resolve_pc_updated;
        return true;
      }
      interpreter->pc = BranchTargetPreferDSO(interpreter, target);
      *pc_updated = true;
      return true;
    }
    if (halfword) {
      WriteReg(interpreter, rd, Load16(interpreter, addr));
    } else if (byte) {
      WriteReg(interpreter, rd, Load8(interpreter, addr));
    } else {
      WriteReg(interpreter, rd, Load32(interpreter, addr));
    }
  } else {
    uint32_t value = ReadReg(interpreter, rd);
    if (halfword) {
      Store16(interpreter, addr, (uint16_t)value);
    } else if (byte) {
      Store8(interpreter, addr, (uint8_t)value);
    } else {
      Store32(interpreter, addr, value);
    }
  }
  if (!preindex) {
    addr = add_offset ? base + offset : base - offset;
    WriteReg(interpreter, rn, addr);
  }
  return true;
}

static bool ExecuteBlockTransfer(ARMInterpreter* interpreter, uint32_t insn,
                                 bool* pc_updated) {
  bool preindex = ((insn >> 24) & 1u) != 0;
  bool add_offset = ((insn >> 23) & 1u) != 0;
  bool writeback = ((insn >> 21) & 1u) != 0;
  bool load = ((insn >> 20) & 1u) != 0;
  int rn = (int)((insn >> 16) & 0xfu);
  uint16_t reglist = (uint16_t)(insn & 0xffffu);
  int count = 0;
  for (int i = 0; i < 16; i++) {
    if ((reglist >> i) & 1) {
      count++;
    }
  }
  uint64_t base = ReadReg(interpreter, rn);
  uint64_t transfer_addr = base;
  uint64_t addr;
  if (!add_offset) {
    transfer_addr = base - (uint64_t)(count * 4);
  }
  addr = transfer_addr;
  for (int i = 0; i < 16; i++) {
    if (((reglist >> i) & 1) == 0) {
      continue;
    }
    if (load) {
      uint32_t value = Load32(interpreter, addr);
      if (i == ARM_PC_REG) {
        interpreter->pc = BranchTarget(interpreter, value);
        *pc_updated = true;
      } else {
        WriteReg(interpreter, i, value);
      }
    } else {
      Store32(interpreter, addr, ReadReg(interpreter, i));
    }
    addr += 4;
  }
  if (writeback) {
    WriteReg(interpreter, rn, add_offset ? addr : transfer_addr);
  }
  return true;
}

static bool ExecuteBranch(ARMInterpreter* interpreter, uint32_t insn,
                          bool* pc_updated) {
  bool link = ((insn >> 24) & 1u) != 0;
  int32_t offset = SignExtend32(insn & 0x00ffffffu, 24) << 2;
  if (link) {
    WriteReg(interpreter, ARM_LR_REG,
             RuntimeToLinked(interpreter, interpreter->pc + 4));
  }
  interpreter->pc = (uint64_t)((int64_t)(interpreter->pc + 8) + offset);
  *pc_updated = true;
  return true;
}

static bool ExecuteBranchExchange(ARMInterpreter* interpreter, uint32_t insn,
                                  bool* pc_updated) {
  bool link = ((insn >> 21) & 1u) != 0;
  int rm = (int)(insn & 0xfu);
  uint64_t target = ReadReg(interpreter, rm);
  if (link) {
    WriteReg(interpreter, ARM_LR_REG,
             RuntimeToLinked(interpreter, interpreter->pc + 4));
  }
  if (target == 0) {
    interpreter->pc = 0;
    *pc_updated = true;
    return true;
  }
  if (target & 1u) {
    fprintf(stderr, "Thumb mode not supported at 0x%08x\n", (uint32_t)target);
    exit(1);
  }
  interpreter->pc = BranchTarget(interpreter, target);
  *pc_updated = true;
  return true;
}

static bool ExecuteSwi(ARMInterpreter* interpreter, uint32_t insn,
                       bool* pc_updated) {
  (void)insn;
  int32_t result =
      HandleSyscall(interpreter, (int32_t)ReadReg(interpreter, ARM_SYSCALL_REG),
                    (int32_t)ReadReg(interpreter, 0),
                    (int32_t)ReadReg(interpreter, 1),
                    (int32_t)ReadReg(interpreter, 2),
                    (int32_t)ReadReg(interpreter, 3),
                    (int32_t)ReadReg(interpreter, 4),
                    (int32_t)ReadReg(interpreter, 5), pc_updated);
  WriteReg(interpreter, 0, (uint64_t)(uint32_t)result);
  return true;
}

static bool ExecuteVfp(ARMInterpreter* interpreter, uint32_t insn) {
  if ((insn & 0x0f000000u) != 0x0e000000u) {
    return false;
  }

  if ((insn & 0x0fb00fffu) == 0x0eb00a40u) {
    int vd = (int)((insn >> 12) & 0xfu);
    int vm = (int)(insn & 0xfu);
    interpreter->sregs[vd] = interpreter->sregs[vm];
    return true;
  }

  if ((insn & 0x0f200000u) == 0x0d000000u) {
    bool load = ((insn >> 20) & 1u) != 0;
    int rn = (int)((insn >> 16) & 0xfu);
    int vd = (int)((insn >> 12) & 0xfu);
    int32_t offset = (int32_t)(insn & 0xffu) * 4;
    uint64_t addr = ReadReg(interpreter, rn) + (uint64_t)offset;
    if (load) {
      uint32_t bits = Load32(interpreter, addr);
      memcpy(&interpreter->sregs[vd], &bits, sizeof(float));
    } else {
      uint32_t bits = 0;
      memcpy(&bits, &interpreter->sregs[vd], sizeof(float));
      Store32(interpreter, addr, bits);
    }
    return true;
  }

  if ((insn & 0x0f000010u) == 0x0e000000u) {
    int op = (int)((insn >> 20) & 0xfu);
    int vn = (int)((insn >> 16) & 0xfu);
    int vd = (int)((insn >> 12) & 0xfu);
    int vm = (int)(insn & 0xfu);
    float lhs = interpreter->sregs[vn];
    float rhs = interpreter->sregs[vm];
    switch (op) {
      case 0x3:
        interpreter->sregs[vd] = lhs + rhs;
        return true;
      case 0x5:
        interpreter->sregs[vd] = lhs - rhs;
        return true;
      case 0x2:
        interpreter->sregs[vd] = lhs * rhs;
        return true;
      case 0x8:
        interpreter->sregs[vd] = lhs / rhs;
        return true;
      default:
        break;
    }
  }

  if ((insn & 0x0fb80f00u) == 0x0eb80a00u) {
    int vn = (int)((insn >> 16) & 0xfu);
    int vd = (int)((insn >> 12) & 0xfu);
    int opc = (int)((insn >> 7) & 0xfu);
    float value = interpreter->sregs[vn];
    switch (opc) {
      case 0x8:
        WriteReg(interpreter, vd, (uint32_t)(int32_t)value);
        return true;
      case 0x0:
        WriteReg(interpreter, vd, (uint32_t)value);
        return true;
      default:
        break;
    }
  }

  if ((insn & 0x0fb00f00u) == 0x0eb40a00u) {
    float lhs = interpreter->sregs[(insn >> 16) & 0xfu];
    float rhs = interpreter->sregs[insn & 0xfu];
    SetNZ(interpreter, (uint32_t)(lhs == rhs ? 0 : 1));
    return true;
  }

  if ((insn & 0x0fff0fffu) == 0x0ef1fa10u) {
    (void)interpreter;
    return true;
  }

  if ((insn & 0x0e100000u) == 0x0e000000u) {
    int rt = (int)((insn >> 12) & 0xfu);
    int vn = (int)((insn >> 16) & 0xfu);
    bool to_arm = ((insn >> 16) & 0x40u) != 0;
    if (to_arm) {
      uint32_t bits = 0;
      memcpy(&bits, &interpreter->sregs[vn], sizeof(float));
      WriteReg(interpreter, rt, bits);
    } else {
      uint32_t bits = ReadReg(interpreter, rt);
      memcpy(&interpreter->sregs[vn], &bits, sizeof(float));
    }
    return true;
  }

  return false;
}

static bool ExecuteInstruction(ARMInterpreter* interpreter, uint32_t insn,
                               bool* pc_updated) {
  *pc_updated = false;
  uint32_t cond = (insn >> 28) & 0xfu;
  if (!ConditionPass(interpreter, cond)) {
    return true;
  }

  if (insn == ARM_BREAKPOINT_INSN) {
    longjmp(interpreter->debugger, 1);
  }

  if ((insn & 0x0f000000u) == 0x0f000000u) {
    return ExecuteSwi(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0e000000u) == 0x0a000000u) {
    return ExecuteBranch(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0ffffff0u) == 0x012ff10u) {
    return ExecuteBranchExchange(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0ff00000u) == 0x03000000u) {
    return ExecuteMovwMovt(interpreter, insn);
  }

  if ((insn & 0x0fc000f0u) == 0x00000090u) {
    return ExecuteMultiply(interpreter, insn);
  }

  if ((insn & 0x0e000000u) == 0x08000000u) {
    return ExecuteBlockTransfer(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0c000000u) == 0x04000000u) {
    return ExecuteLoadStore(interpreter, insn, pc_updated);
  }

  if ((insn & 0x02000000u) != 0) {
    return ExecuteDataProcessingImm(interpreter, insn);
  }

  if ((insn & 0x0e000010u) == 0x00000000u && (insn & 0x02000000u) == 0) {
    return ExecuteDataProcessingReg(interpreter, insn);
  }

  if ((insn & 0x0f000000u) == 0x0e000000u) {
    return ExecuteVfp(interpreter, insn);
  }

  if (insn == 0xE1A00000u) {
    return true;
  }

  return false;
}

void ARMInterpreterDumpRegisters(ARMInterpreter* interpreter) {
  for (int i = 0; i < ARM_NUM_INT_REGS; i += 2) {
    if (i == ARM_PC_REG) {
      continue;
    }
    printf("r%-2d 0x%08" PRIx64 "    ", i, interpreter->regs[i] & 0xffffffffu);
    if (i + 1 < ARM_PC_REG) {
      printf("r%-2d 0x%08" PRIx64 "\n", i + 1,
             interpreter->regs[i + 1] & 0xffffffffu);
    } else {
      printf("\n");
    }
  }
  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i += 2) {
    printf("s%-2d %g        s%-2d %g\n", i, interpreter->sregs[i], i + 1,
           interpreter->sregs[i + 1]);
  }
  printf("sp 0x%08" PRIx64 "  lr 0x%08" PRIx64 "\n",
         interpreter->regs[ARM_SP_REG] & 0xffffffffu,
         interpreter->regs[ARM_LR_REG] & 0xffffffffu);
  printf("pc 0x%016" PRIx64 "  cpsr 0x%08x\n", interpreter->pc,
         interpreter->cpsr);
}

static void MapGuestResolver(ARMInterpreter* interpreter) {
  Loader* loader = interpreter->loader;
  if (loader->is_static || (loader->flags & LOADER_LAZY_RESOLVE) == 0) {
    return;
  }
  int page_size = (int)sysconf(_SC_PAGESIZE);
  void* mem = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
  if (mem == MAP_FAILED) {
    return;
  }
  memcpy(mem, interpreter->symbol_resolver_code,
         sizeof(interpreter->symbol_resolver_code));

  static ELFProgramHeader segment;
  memset(&segment, 0, sizeof(segment));
  segment.vaddr = ARM_RESOLVER_LINKED;
  segment.memsz = (uint64_t)page_size;

  VectorAppend(&loader->regions, NewRegion(mem, 0, page_size, &segment, NULL));

  LoadedDynamicLibrary* main_lib = loader->dynamic_lib;
  if (main_lib == NULL) {
    return;
  }
  const void* pltgot =
      DynamicLoaderFindDynamicSectionAddressEntry(main_lib, DT(pltgot));
  if (pltgot != NULL) {
    ((uint32_t*)pltgot)[0] = ARM_RESOLVER_LINKED;
  }
}

void ARMInterpreterInit(ARMInterpreter* interpreter, Loader* loader,
                        uint64_t entry_address, int argc, char** argv,
                        bool trace_regs, bool trace_instructions) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->trace_regs = trace_regs;
  interpreter->trace_instructions = trace_instructions;
  interpreter->num_steps = -1;

  interpreter->symbol_resolver_code[0] = ARM_AL | (1u << 25) | (0xdu << 21) |
                                         (0u << 16) | (ARM_SYSCALL_REG << 12) |
                                         ARM_SYSCALL_RESOLVE;
  interpreter->symbol_resolver_code[1] = ARM_AL | 0x0f000000u;

  interpreter->stack = malloc(ARM_STACK_SIZE);
  interpreter->regs[ARM_SP_REG] =
      ((uintptr_t)(interpreter->stack + ARM_STACK_SIZE) & ~0x7ull) - 4096;
  interpreter->pc = entry_address;
  interpreter->regs[ARM_LR_REG] = 0;
  WriteReg(interpreter, 0, (uint64_t)(uint32_t)argc);
  if (!loader->is_static) {
    WriteReg(interpreter, 1, entry_address);
  } else {
    WriteReg(interpreter, 1, (uint64_t)(uintptr_t)argv);
  }
  MapGuestResolver(interpreter);
}

void ARMInterpreterCycle(ARMInterpreter* interpreter) {
  for (;;) {
    if (interpreter->pc == 0) {
      exit((int)(ReadReg(interpreter, 0) & 0xffffffffu));
    }
    if (interpreter->num_steps == 0) {
      return;
    }
    if (interpreter->num_steps > 0) {
      --interpreter->num_steps;
    }

    if (interpreter->trace_instructions) {
      interpreter->current_symbol = LoaderFindSymbolAndCacheResult(
          interpreter->loader, interpreter->pc);
      if (interpreter->current_symbol != NULL &&
          interpreter->current_symbol->name != NULL) {
        printf("%s+0x%x: ", interpreter->current_symbol->name,
               (uint32_t)(interpreter->pc -
                          interpreter->current_symbol->start));
      }
    }

    if (interpreter->trace_regs) {
      memcpy(interpreter->old_regs, interpreter->regs,
             sizeof(interpreter->old_regs));
      memcpy(interpreter->old_sregs, interpreter->sregs,
             sizeof(interpreter->old_sregs));
    }

    uint32_t insn = Fetch32(interpreter);
    if (interpreter->trace_instructions) {
      printf("0x%016" PRIx64 ": %08x\n", interpreter->pc, insn);
    }

    bool pc_updated = false;
    if (!ExecuteInstruction(interpreter, insn, &pc_updated)) {
      fprintf(stderr, "Unsupported instruction 0x%08x at 0x%016" PRIx64 "\n",
              insn, interpreter->pc);
      ARMInterpreterDumpRegisters(interpreter);
      exit(1);
    }
    if (!pc_updated) {
      interpreter->pc += 4;
    }

    if (interpreter->trace_regs) {
      for (int i = 0; i < ARM_PC_REG; i++) {
        if (interpreter->regs[i] != interpreter->old_regs[i]) {
          printf("r%d: 0x%08x -> 0x%08x\n", i, interpreter->old_regs[i],
                 interpreter->regs[i]);
        }
      }
    }
  }
}

void ARMInterpreterDestruct(ARMInterpreter* interpreter) {
  free(interpreter->stack);
  interpreter->stack = NULL;
}
