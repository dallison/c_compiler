//
//  arm_interpreter.c
//  arm_interpreter
//

#include "arm_interpreter.h"
#include "arm_process.h"
#include "elf.h"
#include "loader_lifecycle.h"
#include "chrono_host.h"
#include "filesystem_host.h"
#include "random_host.h"
#include <errno.h>
#include "loader_arch.h"
#include "loader_dynamic.h"
#include <fcntl.h>
#include <poll.h>
#include <inttypes.h>
#include <math.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define ARM_RESOLVER_LINKED 0x42000000u

#define CPSR_N (1u << 31)
#define CPSR_Z (1u << 30)
#define CPSR_C (1u << 29)
#define CPSR_V (1u << 28)

static uint64_t RuntimeToLinked(ARMInterpreter* interpreter, uint64_t runtime);

static uint64_t ReadReg(ARMInterpreter* interpreter, int reg) {
  if (reg == ARM_PC_REG) {
    // Hand out the *linked* PC.  Reading PC is how the guest names another
    // address: it adds a link-time displacement to it ('ldr rX, [pc, #n]',
    // and in position-independent code 'add rX, pc, rX').  The interpreter's
    // own pc is a host address, and for an ignore_vaddr target the segments are
    // mapped independently and above 4GB, so a host PC plus a link-time
    // displacement neither addresses the right place once the sum leaves the
    // PC's own segment nor survives being narrowed to a 32-bit register.
    // Every such sum is exact in the linked address space, and the memory and
    // branch paths already translate a linked address when it is used.
    return RuntimeToLinked(interpreter, interpreter->pc) + 8;
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

// Resolve a guest address to a host pointer.  The ARM backend materializes
// absolute *linked* virtual addresses with movw/movt, but the loader maps
// segments at arbitrary host addresses (ignore_vaddr).  An address reaching the
// memory subsystem may therefore be: a stack address (host), an address that is
// already a host/runtime address, or a linked virtual address that must be
// translated to its runtime location.  Returns NULL if unmapped.
static void* ResolveHostPtrExact(ARMInterpreter* interpreter, uint64_t addr,
                                 size_t size) {
  Loader* loader = interpreter->loader;
  if (interpreter->stack != NULL) {
    uint64_t start = interpreter->stack_guest_base;
    uint64_t end = start + ARM_STACK_SIZE;
    if (addr >= start && addr + size <= end) {
      return interpreter->stack + (addr - start);
    }
  }
  if (interpreter->tls_block != NULL && interpreter->tls_block_size > 0) {
    uint64_t start = interpreter->tls_guest_base;
    uint64_t end = start + interpreter->tls_block_size;
    if (addr >= start && addr + size <= end) {
      return (char*)interpreter->tls_block + (addr - start);
    }
  }
  void* process_address =
      ARMProcessResolveGuestAddress(interpreter->process, addr, size);
  if (process_address != NULL) {
    return process_address;
  }
  if (GuestAddressOk(loader, addr, size)) {
    return (void*)(uintptr_t)addr;
  }
  uint64_t host = 0;
  if (LoaderLinkedAddressToRuntime(loader, NULL, addr, &host) &&
      GuestAddressOk(loader, host, size)) {
    return (void*)(uintptr_t)host;
  }
  return NULL;
}

static void* ResolveHostPtr(ARMInterpreter* interpreter, uint64_t addr,
                            size_t size) {
  void* p = ResolveHostPtrExact(interpreter, addr, size);
  if (p != NULL) {
    return p;
  }
  // ARM general-purpose registers are 32 bits wide, but the interpreter holds
  // them in uint64_t slots and computes ALU results in 64 bits.  A 32-bit add
  // that carries past bit 31 therefore leaves stray high bits in the value.
  // When such a value is used as an address, the 32-bit guest address still
  // lives in the low word, so retry the resolution with the high bits cleared.
  // (PC-relative computations legitimately produce >32-bit host addresses and
  // are handled by the exact lookup above, so this only fires as a fallback.)
  if ((addr >> 32) != 0) {
    return ResolveHostPtrExact(interpreter, addr & 0xffffffffu, size);
  }
  return NULL;
}

void* ARMGuestAddressToHost(ARMInterpreter* interpreter, uint64_t addr,
                            size_t size) {
  return ResolveHostPtr(interpreter, addr, size);
}

static uint32_t Fetch32(ARMInterpreter* interpreter) {
  void* p = ResolveHostPtr(interpreter, interpreter->pc, 4);
  if (p == NULL) {
    fprintf(stderr, "Fetch32 outside mapped memory at pc 0x%016" PRIx64 "\n",
            interpreter->pc);
    exit(1);
  }
  return *(uint32_t*)p;
}

static void GuestMemoryLock(ARMInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->memory_mutex);
  }
}

static void GuestMemoryUnlock(ARMInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->memory_mutex);
  }
}

static void GuestMemoryDidWrite(ARMInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    interpreter->process->write_epoch++;
  }
  interpreter->reservation_valid = false;
}

static void Store8(ARMInterpreter* interpreter, uint64_t addr, uint8_t value) {
  void* p = ResolveHostPtr(interpreter, addr, 1);
  if (p == NULL) {
    fprintf(stderr, "Store8 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  *(uint8_t*)p = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static void Store16(ARMInterpreter* interpreter, uint64_t addr, uint16_t value) {
  void* p = ResolveHostPtr(interpreter, addr, 2);
  if (p == NULL) {
    fprintf(stderr, "Store16 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  *(uint16_t*)p = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static void Store32(ARMInterpreter* interpreter, uint64_t addr, uint32_t value) {
  void* p = ResolveHostPtr(interpreter, addr, 4);
  if (p == NULL) {
    fprintf(stderr, "Store32 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  *(uint32_t*)p = value;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
}

static uint8_t Load8(ARMInterpreter* interpreter, uint64_t addr) {
  void* p = ResolveHostPtr(interpreter, addr, 1);
  if (p == NULL) {
    fprintf(stderr, "Load8 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  uint8_t value = *(uint8_t*)p;
  GuestMemoryUnlock(interpreter);
  return value;
}

static uint16_t Load16(ARMInterpreter* interpreter, uint64_t addr) {
  void* p = ResolveHostPtr(interpreter, addr, 2);
  if (p == NULL) {
    fprintf(stderr, "Load16 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  uint16_t value = *(uint16_t*)p;
  GuestMemoryUnlock(interpreter);
  return value;
}

static uint32_t Load32(ARMInterpreter* interpreter, uint64_t addr) {
  void* p = ResolveHostPtr(interpreter, addr, 4);
  if (p == NULL) {
    fprintf(stderr, "Load32 outside mapped memory at 0x%08" PRIx64 "\n", addr);
    exit(1);
  }
  GuestMemoryLock(interpreter);
  uint32_t value = *(uint32_t*)p;
  GuestMemoryUnlock(interpreter);
  return value;
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

// Add-with-carry flag setter.  `carry_in` is 0 or 1.  Computes the result in 64
// bits so the carry-out (and the Z flag, which must reflect the carry-in) are
// correct even when carry_in participates.  Returns the 32-bit result.
static uint32_t SetFlagsAddC(ARMInterpreter* interpreter, uint32_t lhs,
                             uint32_t rhs, uint32_t carry_in) {
  uint64_t wide = (uint64_t)lhs + (uint64_t)rhs + (uint64_t)carry_in;
  uint32_t result = (uint32_t)wide;
  SetNZ(interpreter, result);
  interpreter->cpsr &= ~(CPSR_C | CPSR_V);
  if ((wide >> 32) != 0) {
    interpreter->cpsr |= CPSR_C;
  }
  if (((lhs ^ result) & (rhs ^ result) & 0x80000000u) != 0) {
    interpreter->cpsr |= CPSR_V;
  }
  return result;
}

// Subtract-with-borrow flag setter.  `borrow_in` is 0 or 1 (the inverted carry
// for SBC).  Carry-out is set when no borrow is produced.  Returns the 32-bit
// result.
static uint32_t SetFlagsSubC(ARMInterpreter* interpreter, uint32_t lhs,
                             uint32_t rhs, uint32_t borrow_in) {
  uint64_t wide = (uint64_t)lhs - (uint64_t)rhs - (uint64_t)borrow_in;
  uint32_t result = (uint32_t)wide;
  SetNZ(interpreter, result);
  interpreter->cpsr &= ~(CPSR_C | CPSR_V);
  // No borrow (carry set) iff the 64-bit difference did not go negative.
  if (((wide >> 32) & 1u) == 0) {
    interpreter->cpsr |= CPSR_C;
  }
  if (((lhs ^ rhs) & (lhs ^ result) & 0x80000000u) != 0) {
    interpreter->cpsr |= CPSR_V;
  }
  return result;
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
  bool register_shift = (insn & 0x10u) != 0;
  if (register_shift) {
    shift_amount = (int)(ReadReg(interpreter, (int)((insn >> 8) & 0xfu)) & 0xffu);
  } else {
    shift_amount = (int)((insn >> 7) & 0x1fu);
  }
  if (register_shift && shift_type == 3) {
    shift_amount &= 31;
    if (shift_amount == 0) {
      *operand = value;
      return true;
    }
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
  if (lib->decoded_plt_relocations != NULL) {
    *out = lib->decoded_plt_relocations[index];
    return true;
  }
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

static int64_t PLTRelocationEntrySize(LoadedDynamicLibrary* lib) {
  const DynamicSection* section = lib->dynamic;
  if (section != NULL) {
    for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
      if (section->entries[i].tag == DT(relent) ||
          section->entries[i].tag == DT(relaent)) {
        return (int64_t)section->entries[i].un.val;
      }
    }
  }
  return (int64_t)sizeof(ELFRelocation);
}

static uint64_t BranchTarget(ARMInterpreter* interpreter, uint64_t target) {
  if (target == 0) {
    return 0;
  }
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
  if (target == 0) {
    return 0;
  }
  if (GuestAddressOk(interpreter->loader, target, 4)) {
    return target;
  }
  if (target > 0xffffffffu) {
    return target;
  }
  return LinkedToRuntimePreferDSO(interpreter, (uint32_t)target);
}

static void ResolveAndFixupSymbol(ARMInterpreter* interpreter, bool* pc_updated) {
  // The resolver trampoline saves r7 before loading the private resolver
  // syscall number.  r7 is callee-saved in AAPCS32, so restore it (and sp)
  // before transferring directly to the resolved function.
  uint32_t resolver_sp = (uint32_t)ReadReg(interpreter, ARM_SP_REG);
  uint32_t saved_r7 = Load32(interpreter, resolver_sp);
  WriteReg(interpreter, ARM_SYSCALL_REG, saved_r7);
  WriteReg(interpreter, ARM_SP_REG, resolver_sp + sizeof(uint32_t));

  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->got_resolve_mutex);
  }
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
  int64_t entry_size = PLTRelocationEntrySize(lib);
  int64_t num_relocations =
      entry_size > 0 ? plt_rel_size / entry_size : 0;

  const ELFRelocation* reloc = NULL;
  ELFRelocation file_reloc;
  uint64_t invoked_got_slot = ReadReg(interpreter, ARM_IP_REG);
  for (int64_t i = 0; i < num_relocations; i++) {
    if (!ReadRelaPltEntry(lib, i, &file_reloc)) {
      continue;
    }
    if (ELF_R_TYPE(file_reloc.info) != R_ARM_JUMP_SLOT) {
      continue;
    }
    // The PLT trampoline leaves r12/ip pointing at the GOT slot it used.
    // This identifies the exact relocation even though every unresolved GOT
    // slot initially points at the same resolver stub.  The trampoline reaches
    // the slot from the PC, which the guest sees as a linked address, so the
    // comparison is against the relocation's offset rather than its runtime
    // translation.
    if (file_reloc.offset == invoked_got_slot) {
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
  Store32(interpreter, got_runtime, (uint32_t)symbol->value);
  interpreter->pc = runtime;
  *pc_updated = true;
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->got_resolve_mutex);
  }
}

static int TranslateGuestOpenFlags(int guest_flags) {
  enum {
    kGuestAccmode = 00000003,
    kGuestCreate = 00000100,
    kGuestExclusive = 00000200,
    kGuestNoTty = 00000400,
    kGuestTruncate = 00001000,
    kGuestAppend = 00002000,
    kGuestNonblock = 00004000,
    kGuestSync = 00010000,
    kGuestDirectory = 00200000,
    kGuestNoFollow = 00400000,
  };
  int host = guest_flags & kGuestAccmode;
  if (guest_flags & kGuestCreate) host |= O_CREAT;
  if (guest_flags & kGuestExclusive) host |= O_EXCL;
  if (guest_flags & kGuestNoTty) host |= O_NOCTTY;
  if (guest_flags & kGuestTruncate) host |= O_TRUNC;
  if (guest_flags & kGuestAppend) host |= O_APPEND;
  if (guest_flags & kGuestNonblock) host |= O_NONBLOCK;
  if (guest_flags & kGuestSync) host |= O_SYNC;
  if (guest_flags & kGuestDirectory) host |= O_DIRECTORY;
  if (guest_flags & kGuestNoFollow) host |= O_NOFOLLOW;
  return host;
}

static int32_t HandleSyscall(ARMInterpreter* interpreter, int32_t number,
                             int32_t a0, int32_t a1, int32_t a2, int32_t a3,
                             int32_t a4, int32_t a5, bool* pc_updated) {
  switch (number) {
    case ARM_SYSCALL_HALT:
    case ARM_SYSCALL_EXIT:
      // The syscall number is in r7 and the first argument (the exit status)
      // is in r1 (== a1), matching the other syscalls' argument layout.
      if (interpreter->guest_thread != NULL &&
          !interpreter->guest_thread->is_main) {
        ARMSyscallThreadExit(interpreter->guest_thread, a1);
        *pc_updated = true;
        return a1;
      }
      exit(a1);
      break;
    case ARM_SYSCALL_EXIT_CLEAN:
      if (interpreter->loader != NULL) {
        LoaderLifecycleMarkExecutableFiniComplete(
            interpreter->loader, interpreter->loader->lifecycle);
      }
      WriteReg(interpreter, 0, (uint64_t)(uint32_t)a1);
      interpreter->pc = 0;
      return 0;
    case ARM_SYSCALL_OPEN: {
      void* path = ResolveHostPtr(interpreter, (uint32_t)a1, 1);
      return open((const char*)path, TranslateGuestOpenFlags(a2),
                  (mode_t)a3);
    }
    case ARM_SYSCALL_CLOSE:
      return close(a1);
    case ARM_SYSCALL_READ: {
      void* buf = ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3);
      return (int32_t)read(a1, buf, (size_t)a3);
    }
    case ARM_SYSCALL_WRITE: {
      void* buf = ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3);
      if (getenv("ARM_DBG_SYS")) {
        fprintf(stderr,
                "DBG write fd=%d guest_buf=0x%08x len=%d host_buf=%p\n", a1,
                (uint32_t)a2, a3, buf);
      }
      return (int32_t)write(a1, (const void*)buf, (size_t)a3);
    }
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
    case ARM_SYSCALL_TIME:
      return (int32_t)time(NULL);
    case ARM_SYSCALL_CLOCK:
      return (int32_t)clock();
    case ARM_SYSCALL_THREAD_CREATE:
      return ARMSyscallThreadCreate(interpreter->guest_thread, (uint32_t)a1,
                                    (uint32_t)a2, (uint32_t)a3,
                                    (uint32_t)a4);
    case ARM_SYSCALL_THREAD_JOIN:
      return ARMSyscallThreadJoin(interpreter->guest_thread, (uint32_t)a1,
                                  (uint32_t)a2);
    case ARM_SYSCALL_THREAD_SELF:
      return ARMSyscallThreadSelf(interpreter->guest_thread);
    case ARM_SYSCALL_GET_TP:
      return ARMSyscallGetTp(interpreter->guest_thread);
    case ARM_SYSCALL_THREAD_EXIT:
      ARMSyscallThreadExit(interpreter->guest_thread, a1);
      *pc_updated = true;
      return a1;
    case ARM_SYSCALL_HEAP_LOCK:
      return ARMSyscallHeapLock(interpreter->guest_thread);
    case ARM_SYSCALL_HEAP_UNLOCK:
      return ARMSyscallHeapUnlock(interpreter->guest_thread);
    case ARM_SYSCALL_THREAD_YIELD:
      return sched_yield();
    case ARM_SYSCALL_MONOTONIC_TIME: {
      struct timespec now;
      int64_t* result =
          (int64_t*)ResolveHostPtr(interpreter, (uint32_t)a1, sizeof(int64_t));
      if (result == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    case ARM_SYSCALL_THREAD_DETACH:
      return ARMSyscallThreadDetach(interpreter->guest_thread, (uint32_t)a1);
    case ARM_SYSCALL_ADDR_WAIT:
      return ARMSyscallAddrWait(
          interpreter->guest_thread, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3,
          (int64_t)((uint64_t)(uint32_t)a4 | ((uint64_t)(uint32_t)a5 << 32)));
    case ARM_SYSCALL_ADDR_WAKE:
      return ARMSyscallAddrWake(interpreter->guest_thread, (uint32_t)a1,
                                (uint32_t)a2);
    case ARM_SYSCALL_THREAD_SLEEP:
      return ARMSyscallThreadSleep(interpreter->guest_thread, (uint32_t)a1,
                                    (uint32_t)a2);
    case ARM_SYSCALL_HARDWARE_CONCURRENCY:
      return ARMSyscallHardwareConcurrency();
    case ARM_SYSCALL_REALTIME_TIME: {
      struct timespec now;
      int64_t* result =
          (int64_t*)ResolveHostPtr(interpreter, (uint32_t)a1, sizeof(int64_t));
      if (result == NULL || clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    case ARM_SYSCALL_FS_STATUS:
      return (int32_t)DaveHostFilesystemGetStatus(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1), a2,
          (DaveHostFilesystemStat*)ResolveHostPtr(
              interpreter, (uint32_t)a3, sizeof(DaveHostFilesystemStat)));
    case ARM_SYSCALL_FS_OPEN_DIRECTORY:
      return (int32_t)DaveHostFilesystemOpenDirectory(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1));
    case ARM_SYSCALL_FS_READ_DIRECTORY:
      return (int32_t)DaveHostFilesystemReadDirectory(
          a1, (DaveHostFilesystemDirectoryEntry*)ResolveHostPtr(
                  interpreter, (uint32_t)a2,
                  sizeof(DaveHostFilesystemDirectoryEntry)));
    case ARM_SYSCALL_FS_CLOSE_DIRECTORY:
      return (int32_t)DaveHostFilesystemCloseDirectory(a1);
    case ARM_SYSCALL_FS_CREATE_DIRECTORY:
      return (int32_t)DaveHostFilesystemCreateDirectory(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (uint32_t)a2);
    case ARM_SYSCALL_FS_REMOVE:
      return (int32_t)DaveHostFilesystemRemove(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1));
    case ARM_SYSCALL_FS_RENAME:
      return (int32_t)DaveHostFilesystemRename(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a2, 1));
    case ARM_SYSCALL_FS_CURRENT_PATH:
      return (int32_t)DaveHostFilesystemCurrentPath(
          (char*)ResolveHostPtr(interpreter, (uint32_t)a1, (size_t)a2),
          (size_t)a2);
    case ARM_SYSCALL_FS_SET_CURRENT_PATH:
      return (int32_t)DaveHostFilesystemSetCurrentPath(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1));
    case ARM_SYSCALL_FS_READ_SYMLINK:
      return (int32_t)DaveHostFilesystemReadSymlink(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (char*)ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3),
          (size_t)a3);
    case ARM_SYSCALL_FS_CREATE_SYMLINK:
      return (int32_t)DaveHostFilesystemCreateSymlink(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a2, 1));
    case ARM_SYSCALL_FS_CREATE_HARD_LINK:
      return (int32_t)DaveHostFilesystemCreateHardLink(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a2, 1));
    case ARM_SYSCALL_FS_SET_PERMISSIONS:
      return (int32_t)DaveHostFilesystemSetPermissions(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (uint32_t)a2, a3);
    case ARM_SYSCALL_FS_RESIZE: {
      uint64_t* size = (uint64_t*)ResolveHostPtr(
          interpreter, (uint32_t)a2, sizeof(uint64_t));
      return size == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostFilesystemResize(
                       (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
                       *size);
    }
    case ARM_SYSCALL_FS_SET_MODIFICATION_TIME: {
      int64_t* nanoseconds = (int64_t*)ResolveHostPtr(
          interpreter, (uint32_t)a2, sizeof(int64_t));
      return nanoseconds == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostFilesystemSetModificationTime(
                       (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
                       *nanoseconds);
    }
    case ARM_SYSCALL_FS_SPACE:
      return (int32_t)DaveHostFilesystemQuerySpace(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (DaveHostFilesystemSpace*)ResolveHostPtr(
              interpreter, (uint32_t)a2, sizeof(DaveHostFilesystemSpace)));
    case ARM_SYSCALL_FS_COPY_FILE:
      return (int32_t)DaveHostFilesystemCopyFile(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a2, 1), a3);
    case ARM_SYSCALL_FS_CANONICAL:
      return (int32_t)DaveHostFilesystemCanonical(
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
          (char*)ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3),
          (size_t)a3);
    case ARM_SYSCALL_FS_DESCRIPTOR_STATUS:
      return (int32_t)DaveHostFilesystemGetDescriptorStatus(
          a1, (DaveHostFilesystemStat*)ResolveHostPtr(
                  interpreter, (uint32_t)a2,
                  sizeof(DaveHostFilesystemStat)));
    case ARM_SYSCALL_POLL: {
      nfds_t n = (nfds_t)a2;
      void* fds = ResolveHostPtr(interpreter, (uint32_t)a1,
                                 n * sizeof(struct pollfd));
      return (int32_t)poll((struct pollfd*)fds, n, a3);
    }
    case ARM_SYSCALL_ENVIRONMENT_VALUE: {
      const char* name =
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1);
      char* output =
          (char*)ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3);
      return (int32_t)DaveHostEnvironmentValue(name, output, (size_t)a3);
    }
    case ARM_SYSCALL_TZDB_VERSION:
      return (int32_t)DaveHostChronoTzdbVersion(
          (char*)ResolveHostPtr(interpreter, (uint32_t)a1, (size_t)a2),
          (size_t)a2);
    case ARM_SYSCALL_TZDB_GENERATION: {
      uint64_t* generation = (uint64_t*)ResolveHostPtr(
          interpreter, (uint32_t)a1, sizeof(uint64_t));
      return generation == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostChronoGeneration(generation);
    }
    case ARM_SYSCALL_TZDB_RELOAD: {
      uint64_t* generation = (uint64_t*)ResolveHostPtr(
          interpreter, (uint32_t)a1, sizeof(uint64_t));
      return (int32_t)DaveHostChronoReload(generation);
    }
    case ARM_SYSCALL_TZDB_CURRENT_ZONE:
      return (int32_t)DaveHostChronoCurrentZone(
          (char*)ResolveHostPtr(interpreter, (uint32_t)a1, (size_t)a2),
          (size_t)a2);
    case ARM_SYSCALL_TZDB_ZONE_COUNT: {
      uint32_t* count = (uint32_t*)ResolveHostPtr(
          interpreter, (uint32_t)a1, sizeof(uint32_t));
      return count == NULL ? -DAVE_HOST_EINVAL
                           : (int32_t)DaveHostChronoZoneCount(count);
    }
    case ARM_SYSCALL_TZDB_ZONE_NAME:
      return (int32_t)DaveHostChronoZoneName(
          (uint32_t)a1,
          (char*)ResolveHostPtr(interpreter, (uint32_t)a2, (size_t)a3),
          (size_t)a3);
    case ARM_SYSCALL_TZDB_LOCATE_ZONE: {
      uint32_t* index = (uint32_t*)ResolveHostPtr(
          interpreter, (uint32_t)a4, sizeof(uint32_t));
      return index == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostChronoLocateZone(
                       (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1),
                       (char*)ResolveHostPtr(interpreter, (uint32_t)a2,
                                              (size_t)a3),
                       (size_t)a3, index);
    }
    case ARM_SYSCALL_TZDB_SYS_INFO: {
      const DaveHostChronoSysInfoRequestWire* request =
          (const DaveHostChronoSysInfoRequestWire*)ResolveHostPtr(
              interpreter, (uint32_t)a2, sizeof(DaveHostChronoSysInfoRequestWire));
      if (request == NULL || request->abbrev_capacity == 0) {
        return -DAVE_HOST_EINVAL;
      }
      const char* zone =
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1);
      int64_t* seconds = (int64_t*)ResolveHostPtr(
          interpreter, (uint32_t)request->seconds_address, sizeof(int64_t));
      DaveHostChronoSysInfoWire* result =
          (DaveHostChronoSysInfoWire*)ResolveHostPtr(
              interpreter, (uint32_t)request->result_address,
              sizeof(DaveHostChronoSysInfoWire));
      char* abbrev = (char*)ResolveHostPtr(
          interpreter, (uint32_t)request->abbrev_address,
          request->abbrev_capacity);
      return zone == NULL || seconds == NULL || result == NULL || abbrev == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostChronoSysInfo(
                       zone, *seconds, result, abbrev,
                       request->abbrev_capacity);
    }
    case ARM_SYSCALL_TZDB_LOCAL_INFO: {
      const DaveHostChronoLocalInfoRequestWire* request =
          (const DaveHostChronoLocalInfoRequestWire*)ResolveHostPtr(
              interpreter, (uint32_t)a2, sizeof(DaveHostChronoLocalInfoRequestWire));
      if (request == NULL || request->abbrev_capacity == 0) {
        return -DAVE_HOST_EINVAL;
      }
      const char* zone =
          (const char*)ResolveHostPtr(interpreter, (uint32_t)a1, 1);
      int64_t* seconds = (int64_t*)ResolveHostPtr(
          interpreter, (uint32_t)request->seconds_address, sizeof(int64_t));
      DaveHostChronoLocalInfoWire* result =
          (DaveHostChronoLocalInfoWire*)ResolveHostPtr(
              interpreter, (uint32_t)request->result_address,
              sizeof(DaveHostChronoLocalInfoWire));
      size_t capacity = request->abbrev_capacity;
      char* abbrev = (char*)ResolveHostPtr(
          interpreter, (uint32_t)request->abbrev_address, capacity * 2);
      return zone == NULL || seconds == NULL || result == NULL || abbrev == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostChronoLocalInfo(
                       zone, *seconds, result, abbrev, capacity,
                       abbrev + capacity, capacity);
    }
    case ARM_SYSCALL_TZDB_LEAP_COUNT: {
      uint32_t* count = (uint32_t*)ResolveHostPtr(
          interpreter, (uint32_t)a1, sizeof(uint32_t));
      return count == NULL ? -DAVE_HOST_EINVAL
                           : (int32_t)DaveHostChronoLeapCount(count);
    }
    case ARM_SYSCALL_TZDB_LEAP_INFO:
      return (int32_t)DaveHostChronoLeapInfo(
          (uint32_t)a1,
          (DaveHostChronoLeapSecond*)ResolveHostPtr(
              interpreter, (uint32_t)a2, sizeof(DaveHostChronoLeapSecond)));
    case ARM_SYSCALL_RANDOM_BYTES: {
      size_t size = (size_t)(uint32_t)a2;
      void* buffer = size == 0
                         ? NULL
                         : ResolveHostPtr(interpreter, (uint32_t)a1, size);
      return size != 0 && buffer == NULL
                 ? -DAVE_HOST_EINVAL
                 : (int32_t)DaveHostRandomBytes(buffer, size);
    }
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
    case 0x5: {
      uint32_t cin = (interpreter->cpsr & CPSR_C) ? 1u : 0u;
      if (set_flags) {
        result = SetFlagsAddC(interpreter, (uint32_t)lhs, (uint32_t)rhs, cin);
      } else {
        result = (uint32_t)lhs + (uint32_t)rhs + cin;
      }
      break;
    }
    case 0x6: {
      uint32_t bin = (interpreter->cpsr & CPSR_C) ? 0u : 1u;
      if (set_flags) {
        result = SetFlagsSubC(interpreter, (uint32_t)lhs, (uint32_t)rhs, bin);
      } else {
        result = (uint32_t)lhs - (uint32_t)rhs - bin;
      }
      break;
    }
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
  uint32_t rhs = 0;
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
    case 0x5: {
      uint32_t cin = (interpreter->cpsr & CPSR_C) ? 1u : 0u;
      if (set_flags) {
        result = SetFlagsAddC(interpreter, (uint32_t)lhs, (uint32_t)rhs, cin);
      } else {
        result = (uint32_t)lhs + (uint32_t)rhs + cin;
      }
      break;
    }
    case 0x6: {
      uint32_t bin = (interpreter->cpsr & CPSR_C) ? 0u : 1u;
      if (set_flags) {
        result = SetFlagsSubC(interpreter, (uint32_t)lhs, (uint32_t)rhs, bin);
      } else {
        result = (uint32_t)lhs - (uint32_t)rhs - bin;
      }
      break;
    }
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
  // Multiply-accumulate (mla): cond 0000 001S Rd Ra Rm 1001 Rn adds Ra.
  if (((insn >> 21) & 0x7u) == 0x1u) {
    int ra = (int)((insn >> 12) & 0xfu);
    result += (uint32_t)ReadReg(interpreter, ra);
  }
  WriteReg(interpreter, rd, result);
  return true;
}

// Integer divide: sdiv (cond 0111 0001 Rd 1111 Rm 0001 Rn) and udiv (0111
// 0011 ...) compute Rd = Rn / Rm.  These share bits 27-26 = 01 with the
// load/store encodings, so they must be dispatched before ExecuteLoadStore.
static bool ExecuteDivide(ARMInterpreter* interpreter, uint32_t insn) {
  bool is_unsigned = ((insn >> 21) & 1u) != 0;
  int rd = (int)((insn >> 16) & 0xfu);
  int rm = (int)((insn >> 8) & 0xfu);
  int rn = (int)(insn & 0xfu);
  uint32_t dividend = (uint32_t)ReadReg(interpreter, rn);
  uint32_t divisor = (uint32_t)ReadReg(interpreter, rm);
  uint32_t result;
  if (divisor == 0) {
    result = 0;  // ARM division by zero yields 0.
  } else if (is_unsigned) {
    result = dividend / divisor;
  } else {
    result = (uint32_t)((int32_t)dividend / (int32_t)divisor);
  }
  WriteReg(interpreter, rd, result);
  return true;
}

// Multiply and subtract (mls): cond 0000 0110 Rd Ra Rm 1001 Rn computes
// Rd = Ra - Rn * Rm.
static bool ExecuteMls(ARMInterpreter* interpreter, uint32_t insn) {
  int rd = (int)((insn >> 16) & 0xfu);
  int ra = (int)((insn >> 12) & 0xfu);
  int rm = (int)((insn >> 8) & 0xfu);
  int rn = (int)(insn & 0xfu);
  uint32_t result = (uint32_t)ReadReg(interpreter, ra) -
                    (uint32_t)ReadReg(interpreter, rn) *
                        (uint32_t)ReadReg(interpreter, rm);
  WriteReg(interpreter, rd, result);
  return true;
}

static bool ExecuteMovwMovt(ARMInterpreter* interpreter, uint32_t insn) {
  bool is_movt = ((insn >> 22) & 1u) != 0;
  int rd = (int)((insn >> 12) & 0xfu);
  uint32_t imm16 = (insn & 0xfffu) | ((insn >> 4) & 0xf000u);
  uint32_t value = ReadReg(interpreter, rd);
  if (is_movt) {
    // movt sets the top 16 bits, leaving the bottom 16 untouched.
    value = (value & 0xffffu) | (imm16 << 16);
  } else {
    // movw writes the 16-bit immediate zero-extended: the top 16 bits are
    // cleared (the prior register contents must not survive).
    value = imm16;
  }
  WriteReg(interpreter, rd, value);
  return true;
}

// Halfword transfer: cond 000 P U 1 W L Rn Rt immH 1011 immL.  The 8-bit
// immediate is split across two nibbles.
static bool ExecuteHalfword(ARMInterpreter* interpreter, uint32_t insn) {
  bool preindex = ((insn >> 24) & 1u) != 0;
  (void)preindex;
  bool add_offset = ((insn >> 23) & 1u) != 0;
  bool writeback = ((insn >> 21) & 1u) != 0;
  bool load = ((insn >> 20) & 1u) != 0;
  bool sign = ((insn >> 6) & 1u) != 0;
  bool half = ((insn >> 5) & 1u) != 0;
  int rn = (int)((insn >> 16) & 0xfu);
  int rd = (int)((insn >> 12) & 0xfu);
  uint32_t offset = ((insn >> 4) & 0xf0u) | (insn & 0xfu);
  uint64_t base = ReadReg(interpreter, rn);
  uint64_t addr = base;
  if (preindex) {
    addr = add_offset ? base + offset : base - offset;
    if (writeback) {
      WriteReg(interpreter, rn, addr);
    }
  }
  if (load) {
    uint32_t value;
    if (sign && !half) {
      value = (uint32_t)(int32_t)(int8_t)Load8(interpreter, addr);
    } else if (sign) {
      value = (uint32_t)(int32_t)(int16_t)Load16(interpreter, addr);
    } else {
      value = Load16(interpreter, addr);
    }
    WriteReg(interpreter, rd, value);
  } else {
    Store16(interpreter, addr, (uint16_t)ReadReg(interpreter, rd));
  }
  if (!preindex) {
    addr = add_offset ? base + offset : base - offset;
    WriteReg(interpreter, rn, addr);
  }
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
      if (byte) {
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
    if (byte) {
      WriteReg(interpreter, rd, Load8(interpreter, addr));
    } else {
      WriteReg(interpreter, rd, Load32(interpreter, addr));
    }
  } else {
    uint32_t value = ReadReg(interpreter, rd);
    if (byte) {
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
  // BX has bits [7:4] == 0001, BLX (register) has bits [7:4] == 0011.
  bool link = ((insn >> 4) & 0xfu) == 0x3u;
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
  int32_t number = (int32_t)ReadReg(interpreter, ARM_SYSCALL_REG);
  int32_t result =
      HandleSyscall(interpreter, number,
                    (int32_t)ReadReg(interpreter, 0),
                    (int32_t)ReadReg(interpreter, 1),
                    (int32_t)ReadReg(interpreter, 2),
                    (int32_t)ReadReg(interpreter, 3),
                    (int32_t)ReadReg(interpreter, 4),
                    (int32_t)ReadReg(interpreter, 5), pc_updated);
  // Lazy symbol resolution transfers directly to the resolved function, so
  // preserve r0-r3 as that function's original argument registers.
  if (number != ARM_SYSCALL_RESOLVE) {
    WriteReg(interpreter, 0, (uint64_t)(uint32_t)result);
  }
  return true;
}

// Decode single-precision VFP register fields (Vx 4-bit field + 1 extra bit).
// For single precision the extra bit is the low bit (Sx = Vx<<1 | extra).
static int VfpSd(uint32_t insn) { return (int)(((insn >> 12) & 0xfu) << 1) | (int)((insn >> 22) & 1u); }
static int VfpSn(uint32_t insn) { return (int)(((insn >> 16) & 0xfu) << 1) | (int)((insn >> 7) & 1u); }
static int VfpSm(uint32_t insn) { return (int)(((insn >> 0) & 0xfu) << 1) | (int)((insn >> 5) & 1u); }

// Decode double-precision VFP register fields.  For double precision the extra
// bit is the high bit (Dx = extra<<4 | Vx), naming d0..d31.
static int VfpDd(uint32_t insn) { return (int)((insn >> 12) & 0xfu) | (int)(((insn >> 22) & 1u) << 4); }
static int VfpDn(uint32_t insn) { return (int)((insn >> 16) & 0xfu) | (int)(((insn >> 7) & 1u) << 4); }
static int VfpDm(uint32_t insn) { return (int)((insn >> 0) & 0xfu) | (int)(((insn >> 5) & 1u) << 4); }

// Double-precision register dN aliases the single-precision pair s(2N):s(2N+1).
// The interpreter stores single registers in sregs[]; read/write doubles by
// combining the two halves so the aliasing is correct.
static double ReadDreg(ARMInterpreter* interpreter, int d) {
  uint32_t lo, hi;
  memcpy(&lo, &interpreter->sregs[2 * d], 4);
  memcpy(&hi, &interpreter->sregs[2 * d + 1], 4);
  uint64_t bits = (uint64_t)lo | ((uint64_t)hi << 32);
  double v;
  memcpy(&v, &bits, 8);
  return v;
}

static void WriteDreg(ARMInterpreter* interpreter, int d, double v) {
  uint64_t bits;
  memcpy(&bits, &v, 8);
  uint32_t lo = (uint32_t)bits;
  uint32_t hi = (uint32_t)(bits >> 32);
  memcpy(&interpreter->sregs[2 * d], &lo, 4);
  memcpy(&interpreter->sregs[2 * d + 1], &hi, 4);
}

// Read/write the raw 64-bit pattern of dN (used by vmov Dm,Rt,Rt2 and vldr/vstr).
static uint64_t ReadDregBits(ARMInterpreter* interpreter, int d) {
  uint32_t lo, hi;
  memcpy(&lo, &interpreter->sregs[2 * d], 4);
  memcpy(&hi, &interpreter->sregs[2 * d + 1], 4);
  return (uint64_t)lo | ((uint64_t)hi << 32);
}

static void WriteDregBits(ARMInterpreter* interpreter, int d, uint64_t bits) {
  uint32_t lo = (uint32_t)bits;
  uint32_t hi = (uint32_t)(bits >> 32);
  memcpy(&interpreter->sregs[2 * d], &lo, 4);
  memcpy(&interpreter->sregs[2 * d + 1], &hi, 4);
}

// Set the CPSR N/Z/C/V flags from a floating-point comparison, matching the
// ARM FPSCR->APSR_nzcv mapping used after vcmp.
static void SetFloatCompareFlags(ARMInterpreter* interpreter, double lhs,
                                 double rhs) {
  interpreter->cpsr &= ~(CPSR_N | CPSR_Z | CPSR_C | CPSR_V);
  if (lhs < rhs) {
    interpreter->cpsr |= CPSR_N;                       // less than
  } else if (lhs == rhs) {
    interpreter->cpsr |= CPSR_Z | CPSR_C;              // equal
  } else if (lhs > rhs) {
    interpreter->cpsr |= CPSR_C;                       // greater than
  } else {
    interpreter->cpsr |= CPSR_C | CPSR_V;              // unordered (NaN)
  }
}

static bool ExecuteVfp(ARMInterpreter* interpreter, uint32_t insn) {
  if ((insn & 0x0f000000u) != 0x0e000000u &&
      (insn & 0x0f200000u) != 0x0d000000u &&
      (insn & 0x0fe00000u) != 0x0c400000u) {
    return false;
  }

  // vmov Dm, Rt, Rt2 (to FP) / vmov Rt, Rt2, Dm (from FP): doubleword transfer
  // between two core registers and a double-precision register.  Encoding
  // cccc 1100 010L Rt2 Rt 1011 00M1 Vm.
  if ((insn & 0x0fe00fd0u) == 0x0c400b10u) {
    bool to_arm = ((insn >> 20) & 1u) != 0;
    int rt = (int)((insn >> 12) & 0xfu);
    int rt2 = (int)((insn >> 16) & 0xfu);
    int dm = (int)(insn & 0xfu) | (int)(((insn >> 5) & 1u) << 4);
    if (to_arm) {
      uint64_t bits = ReadDregBits(interpreter, dm);
      WriteReg(interpreter, rt, (uint32_t)bits);
      WriteReg(interpreter, rt2, (uint32_t)(bits >> 32));
    } else {
      uint64_t lo = (uint32_t)ReadReg(interpreter, rt);
      uint64_t hi = (uint32_t)ReadReg(interpreter, rt2);
      WriteDregBits(interpreter, dm, lo | (hi << 32));
    }
    return true;
  }

  // Custom int<->float conversions, encoded in coprocessor 11 (0xb).
  // Bit 7 (free in this custom encoding) marks double-precision operands.
  if ((insn & 0x0f000f10u) == 0x0e000b10u) {
    bool is_double = ((insn >> 7) & 1u) != 0;
    if ((insn & (1u << 20)) == 0) {
      // scvtf sd, rt : signed int (or ucvtf if bit22 set) -> float.
      int rt = (int)((insn >> 16) & 0xfu);
      uint32_t bits = (uint32_t)ReadReg(interpreter, rt);
      bool is_unsigned = (insn & (1u << 22)) != 0;
      if (is_double) {
        int dd = VfpDd(insn);
        WriteDreg(interpreter, dd,
                  is_unsigned ? (double)bits : (double)(int32_t)bits);
      } else {
        int sd = VfpSd(insn);
        interpreter->sregs[sd] =
            is_unsigned ? (float)bits : (float)(int32_t)bits;
      }
    } else {
      // fcvtnu/fcvtns rt, sm : float -> int (round toward zero).
      int rt = (int)((insn >> 12) & 0xfu);
      double v = is_double ? ReadDreg(interpreter, VfpDm(insn))
                           : (double)interpreter->sregs[VfpSm(insn)];
      WriteReg(interpreter, rt, (uint32_t)(int32_t)v);
    }
    return true;
  }

  // vldr/vstr sd, [rn, #+/-imm]  (bit 8 selects double precision)
  if ((insn & 0x0f200000u) == 0x0d000000u) {
    bool load = ((insn >> 20) & 1u) != 0;
    bool add = ((insn >> 23) & 1u) != 0;
    bool is_double = ((insn >> 8) & 1u) != 0;
    int rn = (int)((insn >> 16) & 0xfu);
    int32_t offset = (int32_t)(insn & 0xffu) * 4;
    if (!add) {
      offset = -offset;
    }
    uint64_t addr = ReadReg(interpreter, rn) + (uint64_t)(int64_t)offset;
    if (is_double) {
      int dd = VfpDd(insn);
      if (load) {
        uint64_t bits = (uint64_t)Load32(interpreter, addr) |
                        ((uint64_t)Load32(interpreter, addr + 4) << 32);
        WriteDregBits(interpreter, dd, bits);
      } else {
        uint64_t bits = ReadDregBits(interpreter, dd);
        Store32(interpreter, addr, (uint32_t)bits);
        Store32(interpreter, addr + 4, (uint32_t)(bits >> 32));
      }
    } else {
      int sd = VfpSd(insn);
      if (load) {
        uint32_t bits = Load32(interpreter, addr);
        memcpy(&interpreter->sregs[sd], &bits, sizeof(float));
      } else {
        uint32_t bits = 0;
        memcpy(&bits, &interpreter->sregs[sd], sizeof(float));
        Store32(interpreter, addr, bits);
      }
    }
    return true;
  }

  // vmov sn, rt / vmov rt, sn (GP register <-> single register, raw bits).
  if ((insn & 0x0fe00f10u) == 0x0e000a10u) {
    bool to_arm = ((insn >> 20) & 1u) != 0;
    int rt = (int)((insn >> 12) & 0xfu);
    int sn = VfpSn(insn);
    if (to_arm) {
      uint32_t bits = 0;
      memcpy(&bits, &interpreter->sregs[sn], sizeof(float));
      WriteReg(interpreter, rt, bits);
    } else {
      uint32_t bits = (uint32_t)ReadReg(interpreter, rt);
      memcpy(&interpreter->sregs[sn], &bits, sizeof(float));
    }
    return true;
  }

  // vmrs APSR_nzcv, FPSCR : flags already set by vcmp, so this is a no-op.
  if ((insn & 0x0fffffffu) == 0x0ef1fa10u) {
    return true;
  }

  // vcmp sd, sm (bit 8 selects double precision)
  if ((insn & 0x0fbf0ed0u) == 0x0eb40a40u) {
    if (((insn >> 8) & 1u) != 0) {
      SetFloatCompareFlags(interpreter, ReadDreg(interpreter, VfpDd(insn)),
                           ReadDreg(interpreter, VfpDm(insn)));
    } else {
      SetFloatCompareFlags(interpreter, interpreter->sregs[VfpSd(insn)],
                           interpreter->sregs[VfpSm(insn)]);
    }
    return true;
  }

  // vcvt, vneg, vabs, vsqrt, vmov(copy) : 1110 1110 1D11 op Vd 101 sz x1.0 Vm.
  if ((insn & 0x0fb00e50u) == 0x0eb00a40u) {
    bool is_double = ((insn >> 8) & 1u) != 0;
    int op = (int)((insn >> 16) & 0xfu);  // distinguishing field
    bool bit7 = ((insn >> 7) & 1u) != 0;
    if (is_double) {
      int dd = VfpDd(insn);
      int dm = VfpDm(insn);
      switch (op) {
        case 0x0:  // vmov copy (bit7=0) or vabs (bit7=1)
          WriteDreg(interpreter, dd,
                    bit7 ? fabs(ReadDreg(interpreter, dm))
                         : ReadDreg(interpreter, dm));
          return true;
        case 0x1:  // vneg (bit7=0) or vsqrt (bit7=1)
          WriteDreg(interpreter, dd,
                    bit7 ? sqrt(ReadDreg(interpreter, dm))
                         : -ReadDreg(interpreter, dm));
          return true;
        case 0x7:  // vcvt.f32.f64 : double in dm -> float in sd (sz=1 source)
          interpreter->sregs[VfpSd(insn)] = (float)ReadDreg(interpreter, dm);
          return true;
        case 0x8:  // vcvt.f64.s32/.u32 : int bits in sm -> double in dd
          if (bit7) {
            WriteDreg(interpreter, dd,
                      (double)(int32_t)interpreter->sregs[VfpSm(insn)]);
          } else {
            uint32_t b;
            memcpy(&b, &interpreter->sregs[VfpSm(insn)], 4);
            WriteDreg(interpreter, dd, (double)b);
          }
          return true;
        case 0xc:  // vcvt.u32.f64 : double in dm -> unsigned int bits in sd
        case 0xd: {  // vcvt.s32.f64 : double in dm -> int bits in sd
          double v = ReadDreg(interpreter, dm);
          uint32_t bits = (op == 0xd) ? (uint32_t)(int32_t)v : (uint32_t)v;
          memcpy(&interpreter->sregs[VfpSd(insn)], &bits, 4);
          return true;
        }
        default:
          break;
      }
    } else {
      int sd = VfpSd(insn);
      int sm = VfpSm(insn);
      switch (op) {
        case 0x0:  // vmov copy (bit7=0) or vabs (bit7=1)
          interpreter->sregs[sd] =
              bit7 ? fabsf(interpreter->sregs[sm]) : interpreter->sregs[sm];
          return true;
        case 0x1:  // vneg (bit7=0) or vsqrt (bit7=1)
          interpreter->sregs[sd] =
              bit7 ? sqrtf(interpreter->sregs[sm]) : -interpreter->sregs[sm];
          return true;
        case 0x7:  // vcvt.f64.f32 : float in sm -> double in dd (sz=0 source)
          WriteDreg(interpreter, VfpDd(insn), (double)interpreter->sregs[sm]);
          return true;
        case 0x8:  // vcvt.f32.s32 : int bits in sm -> float in sd
          interpreter->sregs[sd] = (float)(int32_t)interpreter->sregs[sm];
          return true;
        case 0xd: {  // vcvt.s32.f32 : float in sm -> int bits in sd
          int32_t v = (int32_t)interpreter->sregs[sm];
          memcpy(&interpreter->sregs[sd], &v, sizeof(int32_t));
          return true;
        }
        default:
          break;
      }
    }
  }

  // vadd/vsub/vmul/vdiv sd, sn, sm (bit 8 selects double precision)
  if ((insn & 0x0f000e10u) == 0x0e000a00u) {
    uint32_t opc = (insn >> 20) & 0xfu;
    bool op6 = ((insn >> 6) & 1u) != 0;
    bool is_double = ((insn >> 8) & 1u) != 0;
    if (is_double) {
      double lhs = ReadDreg(interpreter, VfpDn(insn));
      double rhs = ReadDreg(interpreter, VfpDm(insn));
      int dd = VfpDd(insn);
      if (opc == 0x3 && !op6) {
        WriteDreg(interpreter, dd, lhs + rhs);
      } else if (opc == 0x3 && op6) {
        WriteDreg(interpreter, dd, lhs - rhs);
      } else if (opc == 0x2) {
        WriteDreg(interpreter, dd, lhs * rhs);
      } else if (opc == 0x8) {
        WriteDreg(interpreter, dd, lhs / rhs);
      } else {
        return false;
      }
      return true;
    }
    int sd = VfpSd(insn);
    float lhs = interpreter->sregs[VfpSn(insn)];
    float rhs = interpreter->sregs[VfpSm(insn)];
    if (opc == 0x3 && !op6) {
      interpreter->sregs[sd] = lhs + rhs;  // vadd
    } else if (opc == 0x3 && op6) {
      interpreter->sregs[sd] = lhs - rhs;  // vsub
    } else if (opc == 0x2) {
      interpreter->sregs[sd] = lhs * rhs;  // vmul
    } else if (opc == 0x8) {
      interpreter->sregs[sd] = lhs / rhs;  // vdiv
    } else {
      return false;
    }
    return true;
  }

  return false;
}

static bool ExecuteExclusive(ARMInterpreter* interpreter, uint32_t insn) {
  uint32_t load_key = insn & 0x0ff00fffu;
  uint32_t store_key = insn & 0x0ff00ff0u;
  bool load = load_key == 0x01900f9fu ||
              load_key == 0x01d00f9fu ||
              load_key == 0x01f00f9fu;
  bool store = store_key == 0x01800f90u ||
               store_key == 0x01c00f90u ||
               store_key == 0x01e00f90u;
  if (!load && !store) {
    return false;
  }

  size_t size;
  if (load ? load_key == 0x01d00f9fu : store_key == 0x01c00f90u) {
    size = 1;
  } else if (load ? load_key == 0x01f00f9fu
                  : store_key == 0x01e00f90u) {
    size = 2;
  } else {
    size = 4;
  }
  int rn = (int)((insn >> 16) & 0xfu);
  uint64_t addr = ReadReg(interpreter, rn);
  void* p = ResolveHostPtr(interpreter, addr, size);
  if (p == NULL) {
    fprintf(stderr, "Exclusive access outside mapped memory at 0x%08" PRIx64
                    "\n", addr);
    exit(1);
  }

  GuestMemoryLock(interpreter);
  if (load) {
    int rt = (int)((insn >> 12) & 0xfu);
    uint32_t value = size == 1 ? *(uint8_t*)p
                     : size == 2 ? *(uint16_t*)p
                                 : *(uint32_t*)p;
    WriteReg(interpreter, rt, value);
    interpreter->reservation_valid = true;
    interpreter->reservation_address = addr;
    interpreter->reservation_size = size;
    interpreter->reservation_epoch =
        interpreter->process != NULL ? interpreter->process->write_epoch : 0;
    GuestMemoryUnlock(interpreter);
    return true;
  }

  int rd = (int)((insn >> 12) & 0xfu);
  int rt = (int)(insn & 0xfu);
  bool success = interpreter->reservation_valid &&
                 interpreter->reservation_address == addr &&
                 interpreter->reservation_size == size &&
                 (interpreter->process == NULL ||
                  interpreter->reservation_epoch ==
                      interpreter->process->write_epoch);
  interpreter->reservation_valid = false;
  if (success) {
    uint32_t value = (uint32_t)ReadReg(interpreter, rt);
    if (size == 1) {
      *(uint8_t*)p = (uint8_t)value;
    } else if (size == 2) {
      *(uint16_t*)p = (uint16_t)value;
    } else {
      *(uint32_t*)p = value;
    }
    if (interpreter->process != NULL) {
      interpreter->process->write_epoch++;
    }
  }
  WriteReg(interpreter, rd, success ? 0 : 1);
  GuestMemoryUnlock(interpreter);
  return true;
}

static void ReadNeonBits(ARMInterpreter* interpreter, int d, bool q,
                         uint8_t out[16]) {
  memset(out, 0, 16);
  memcpy(out, &interpreter->sregs[2 * d], q ? 16 : 8);
}

static void WriteNeonBits(ARMInterpreter* interpreter, int d, bool q,
                          const uint8_t in[16]) {
  memcpy(&interpreter->sregs[2 * d], in, q ? 16 : 8);
}

static uint64_t NeonLoadLane(const uint8_t* v, int elem, int i) {
  uint64_t value = 0;
  memcpy(&value, v + (size_t)i * (size_t)elem, (size_t)elem);
  return value;
}

static void NeonStoreLane(uint8_t* v, int elem, int i, uint64_t value) {
  memcpy(v + (size_t)i * (size_t)elem, &value, (size_t)elem);
}

static int64_t NeonSignExtend(uint64_t value, int elem) {
  int bits = elem * 8;
  int shift = 64 - bits;
  return (int64_t)(value << shift) >> shift;
}

static bool ExecuteNeon(ARMInterpreter* interpreter, uint32_t insn) {
  // Advanced SIMD load/store: 1111 0100 0 UL ... vld1/vst1.
  if ((insn & 0xff000000u) == 0xf4000000u) {
    bool load = ((insn >> 21) & 1u) != 0;
    int rn = (int)((insn >> 16) & 0xfu);
    int d = VfpDd(insn);
    uint32_t type = (insn >> 8) & 0xfu;
    bool q = type == 0xau;
    uint64_t addr = ReadReg(interpreter, rn);
    int bytes = q ? 16 : 8;
    if (load) {
      uint8_t bits[16];
      memset(bits, 0, sizeof(bits));
      for (int i = 0; i < bytes; i += 4) {
        uint32_t word = Load32(interpreter, addr + (uint64_t)i);
        memcpy(bits + i, &word, 4);
      }
      WriteNeonBits(interpreter, d, q, bits);
    } else {
      uint8_t bits[16];
      ReadNeonBits(interpreter, d, q, bits);
      for (int i = 0; i < bytes; i += 4) {
        uint32_t word;
        memcpy(&word, bits + i, 4);
        Store32(interpreter, addr + (uint64_t)i, word);
      }
    }
    return true;
  }

  if ((insn & 0xfe000000u) != 0xf2000000u) {
    return false;
  }

  bool q = ((insn >> 6) & 1u) != 0;
  int size = (int)((insn >> 20) & 3u);
  int dd = VfpDd(insn);
  int dn = VfpDn(insn);
  int dm = VfpDm(insn);
  int bytes = q ? 16 : 8;
  uint8_t left[16];
  uint8_t right[16];
  uint8_t result[16];
  ReadNeonBits(interpreter, dn, q, left);
  ReadNeonBits(interpreter, dm, q, right);
  memset(result, 0, sizeof(result));

  if ((insn & 0xfea00f10u) == 0xf2000d00u ||
      (insn & 0xfea00f10u) == 0xf2200d00u ||
      (insn & 0xfea00f10u) == 0xf2000d10u) {
    bool is_sub = (insn & 0xfea00f10u) == 0xf2200d00u;
    bool is_mul = (insn & 0xfea00f10u) == 0xf2000d10u;
    bool f64 = size == 1;
    int elem = f64 ? 8 : 4;
    int lanes = bytes / elem;
    for (int i = 0; i < lanes; i++) {
      if (f64) {
        double a, b, r;
        memcpy(&a, left + i * 8, 8);
        memcpy(&b, right + i * 8, 8);
        r = is_mul ? a * b : is_sub ? a - b : a + b;
        memcpy(result + i * 8, &r, 8);
      } else {
        float a, b, r;
        memcpy(&a, left + i * 4, 4);
        memcpy(&b, right + i * 4, 4);
        r = is_mul ? a * b : is_sub ? a - b : a + b;
        memcpy(result + i * 4, &r, 4);
      }
    }
    WriteNeonBits(interpreter, dd, q, result);
    return true;
  }

  int elem = 1 << size;
  if (elem > bytes) {
    return false;
  }
  int lanes = bytes / elem;
  enum {
    kNeonAdd,
    kNeonSub,
    kNeonAnd,
    kNeonOrr,
    kNeonEor,
    kNeonCeq,
    kNeonCgtS,
    kNeonCgeS,
    kNeonCgtU,
    kNeonCgeU
  } op;
  if ((insn & 0xff800f10u) == 0xf2000800u) {
    op = kNeonAdd;
  } else if ((insn & 0xff800f10u) == 0xf3000800u) {
    op = kNeonSub;
  } else if ((insn & 0xffb00f10u) == 0xf2000110u) {
    op = kNeonAnd;
    elem = 1;
    lanes = bytes;
  } else if ((insn & 0xffb00f10u) == 0xf2200110u) {
    op = kNeonOrr;
    elem = 1;
    lanes = bytes;
  } else if ((insn & 0xffb00f10u) == 0xf3000110u) {
    op = kNeonEor;
    elem = 1;
    lanes = bytes;
  } else if ((insn & 0xff800f10u) == 0xf3000810u) {
    op = kNeonCeq;
  } else if ((insn & 0xff800f10u) == 0xf2000300u) {
    op = kNeonCgtS;
  } else if ((insn & 0xff800f10u) == 0xf3000300u) {
    op = kNeonCgeS;
  } else if ((insn & 0xff800f10u) == 0xf2000310u) {
    op = kNeonCgtU;
  } else if ((insn & 0xff800f10u) == 0xf3000310u) {
    op = kNeonCgeU;
  } else {
    return false;
  }

  uint64_t mask = elem == 8 ? ~0ull : ((1ull << (elem * 8)) - 1ull);
  uint64_t all_ones = mask;
  for (int i = 0; i < lanes; i++) {
    uint64_t a = NeonLoadLane(left, elem, i) & mask;
    uint64_t b = NeonLoadLane(right, elem, i) & mask;
    uint64_t r = 0;
    switch (op) {
      case kNeonAdd:
        r = (a + b) & mask;
        break;
      case kNeonSub:
        r = (a - b) & mask;
        break;
      case kNeonAnd:
        r = a & b;
        break;
      case kNeonOrr:
        r = a | b;
        break;
      case kNeonEor:
        r = a ^ b;
        break;
      case kNeonCeq:
        r = a == b ? all_ones : 0;
        break;
      case kNeonCgtS:
        r = NeonSignExtend(a, elem) > NeonSignExtend(b, elem) ? all_ones : 0;
        break;
      case kNeonCgeS:
        r = NeonSignExtend(a, elem) >= NeonSignExtend(b, elem) ? all_ones : 0;
        break;
      case kNeonCgtU:
        r = a > b ? all_ones : 0;
        break;
      case kNeonCgeU:
        r = a >= b ? all_ones : 0;
        break;
    }
    NeonStoreLane(result, elem, i, r);
  }
  WriteNeonBits(interpreter, dd, q, result);
  return true;
}

static bool ExecuteInstruction(ARMInterpreter* interpreter, uint32_t insn,
                               bool* pc_updated) {
  *pc_updated = false;
  uint32_t cond = (insn >> 28) & 0xfu;
  if (!ConditionPass(interpreter, cond)) {
    return true;
  }

  // Unconditional NEON (top nibble 0xF): three-same 0xF2/F3, vld1/vst1 0xF4.
  if ((insn & 0xfe000000u) == 0xf2000000u ||
      (insn & 0xff000000u) == 0xf4000000u) {
    return ExecuteNeon(interpreter, insn);
  }

  if (insn == ARM_BREAKPOINT_INSN) {
    longjmp(interpreter->debugger, 1);
  }

  if ((insn & 0xffff0fffu) == 0xee1d0f70u) {
    WriteReg(interpreter, (int)((insn >> 12) & 0xfu),
             interpreter->tp_base);
    return true;
  }

  if ((insn & 0xfffffff0u) == 0xf57ff050u) {
    atomic_thread_fence(memory_order_seq_cst);
    return true;
  }
  if (insn == 0xf57ff01fu) {
    interpreter->reservation_valid = false;
    return true;
  }
  if (ExecuteExclusive(interpreter, insn)) {
    return true;
  }

  if ((insn & 0x0f000000u) == 0x0f000000u) {
    return ExecuteSwi(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0e000000u) == 0x0a000000u) {
    return ExecuteBranch(interpreter, insn, pc_updated);
  }

  // BX:  cond 0001 0010 1111 1111 1111 0001 Rm
  // BLX: cond 0001 0010 1111 1111 1111 0011 Rm
  if ((insn & 0x0ffffff0u) == 0x012fff10u ||
      (insn & 0x0ffffff0u) == 0x012fff30u) {
    return ExecuteBranchExchange(interpreter, insn, pc_updated);
  }

  // movw (0x03000000) and movt (0x03400000); bit 22 selects movt.
  if ((insn & 0x0fb00000u) == 0x03000000u) {
    return ExecuteMovwMovt(interpreter, insn);
  }

  if ((insn & 0x0fc000f0u) == 0x00000090u) {
    return ExecuteMultiply(interpreter, insn);
  }

  // Integer divide (sdiv/udiv): cond 0111 00x1 Rd 1111 Rm 0001 Rn.  Bits 27-26
  // are 01, shared with the load/store encodings, so dispatch this first.
  if ((insn & 0x0f9000f0u) == 0x07100010u) {
    return ExecuteDivide(interpreter, insn);
  }

  if ((insn & 0x0fff0ff0u) == 0x016f0f10u) {  // CLZ
    int rd = (insn >> 12) & 0xf;
    int rm = insn & 0xf;
    uint32_t value = (uint32_t)ReadReg(interpreter, rm);
    WriteReg(interpreter, rd,
             value == 0 ? 32u : (uint32_t)__builtin_clz(value));
    return true;
  }

  if ((insn & 0x0fff0ff0u) == 0x06ff0f30u) {  // RBIT
    int rd = (insn >> 12) & 0xf;
    int rm = insn & 0xf;
    uint32_t value = (uint32_t)ReadReg(interpreter, rm);
    uint32_t result = 0;
    for (int i = 0; i < 32; i++) {
      result = (result << 1) | ((value >> i) & 1u);
    }
    WriteReg(interpreter, rd, result);
    return true;
  }

  // Multiply and subtract (mls): cond 0000 0110 Rd Ra Rm 1001 Rn.
  if ((insn & 0x0ff000f0u) == 0x00600090u) {
    return ExecuteMls(interpreter, insn);
  }

  // Halfword and signed byte/halfword transfers.
  if ((insn & 0x0e000090u) == 0x00000090u) {
    return ExecuteHalfword(interpreter, insn);
  }

  if ((insn & 0x0e000000u) == 0x08000000u) {
    return ExecuteBlockTransfer(interpreter, insn, pc_updated);
  }

  if ((insn & 0x0c000000u) == 0x04000000u) {
    return ExecuteLoadStore(interpreter, insn, pc_updated);
  }

  // Coprocessor / VFP space: bits 27-26 = 11 (0xc/0xd/0xe).  This must be
  // tested before the data-processing checks below: VFP load/store (vldr/vstr,
  // 0x0d......) and VFP data ops set bit 25, so they would otherwise be
  // mis-dispatched as data-processing-immediate instructions.
  if ((insn & 0x0c000000u) == 0x0c000000u) {
    return ExecuteVfp(interpreter, insn);
  }

  if ((insn & 0x02000000u) != 0) {
    return ExecuteDataProcessingImm(interpreter, insn);
  }

  // Data-processing register form: bits 27-25 == 000.  This covers both the
  // immediate-shift form (bit 4 == 0) and the register-specified-shift form
  // (bit 4 == 1, bit 7 == 0).  Multiply, divide, mls, halfword and branch-
  // exchange encodings share these bits but are all dispatched above, so by the
  // time we get here a 000 class instruction is a data-processing-register op.
  if ((insn & 0x0e000000u) == 0x00000000u) {
    return ExecuteDataProcessingReg(interpreter, insn);
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

  uint64_t plt_linked = 0;
  uint64_t plt_size = 0;
  uint64_t reloc_linked = 0;
  uint64_t reloc_size = 0;
  bool found_relocations =
      SectionAddressAndSize(main_lib, ".rel.plt", &reloc_linked,
                            &reloc_size) ||
      SectionAddressAndSize(main_lib, ".rela.plt", &reloc_linked,
                            &reloc_size);
  if (!SectionAddressAndSize(main_lib, ".plt", &plt_linked, &plt_size) ||
      !found_relocations) {
    return;
  }
  (void)reloc_linked;
  int64_t entry_size = PLTRelocationEntrySize(main_lib);
  int64_t num_relocations =
      entry_size > 0 ? (int64_t)(reloc_size / (uint64_t)entry_size) : 0;
  for (int64_t i = 0; i < num_relocations; i++) {
    ELFRelocation reloc;
    if (!ReadRelaPltEntry(main_lib, i, &reloc) ||
        ELF_R_TYPE(reloc.info) != R_ARM_JUMP_SLOT) {
      continue;
    }
    uint64_t got_slot = 0;
    if (!LoaderLinkedAddressToRuntime(loader, main_lib, reloc.offset,
                                      &got_slot)) {
      continue;
    }
    uint32_t target = *(uint32_t*)(uintptr_t)got_slot;
    if (target >= plt_linked && target < plt_linked + plt_size) {
      *(uint32_t*)(uintptr_t)got_slot = ARM_RESOLVER_LINKED;
    }
  }
}

static uint64_t ARMInterpreterInitialSp(ARMInterpreter* interpreter) {
  return (((uint64_t)interpreter->stack_guest_base + ARM_STACK_SIZE) &
          ~0x7ull) -
         4096;
}

static void SetupGuestMainArgs(ARMInterpreter* interpreter, Loader* loader,
                               int argc, char** argv, uint64_t entry_address,
                               bool is_static_link) {
  (void)loader;
  (void)entry_address;
  (void)is_static_link;
  WriteReg(interpreter, 0, (uint64_t)(uint32_t)argc);
  if (argc > 0 && argv != NULL) {
    const size_t argument_space = 4096;
    if ((size_t)argc >= argument_space / sizeof(uint32_t)) {
      goto invalid_args;
    }
    size_t string_bytes = 0;
    for (int i = 0; i < argc; ++i) {
      size_t len = strlen(argv[i]) + 1;
      if (len > argument_space - string_bytes) {
        goto invalid_args;
      }
      string_bytes += len;
    }
    size_t vector_bytes = (size_t)(argc + 1) * sizeof(uint32_t);
    if (string_bytes + vector_bytes + 15 > argument_space) {
      goto invalid_args;
    }

    uint64_t guest_top =
        ((uint64_t)interpreter->stack_guest_base + ARM_STACK_SIZE) & ~0x7ull;
    uint64_t p = guest_top;
    uint32_t* guest_ptrs = malloc((size_t)(argc + 1) * sizeof(uint32_t));
    if (guest_ptrs == NULL) {
      goto invalid_args;
    }
    for (int i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]) + 1;
      p -= len;
      memcpy(interpreter->stack + (p - interpreter->stack_guest_base), argv[i],
             len);
      guest_ptrs[i] = (uint32_t)p;
    }
    guest_ptrs[argc] = 0;
    p &= ~0x7ull;
    p -= (uint64_t)(argc + 1) * sizeof(uint32_t);
    p &= ~0x7ull;
    uint64_t guest_argv = p;
    memcpy(interpreter->stack +
               (guest_argv - interpreter->stack_guest_base),
           guest_ptrs,
           (size_t)(argc + 1) * sizeof(uint32_t));
    free(guest_ptrs);
    WriteReg(interpreter, 1, guest_argv);
  } else {
    WriteReg(interpreter, 1, 0);
  }
  return;

invalid_args:
  WriteReg(interpreter, 0, 0);
  WriteReg(interpreter, 1, 0);
}

void ARMInterpreterInitForThread(
    ARMInterpreter* interpreter, ARMProcessRuntime* process,
    ARMGuestThread* guest_thread, Loader* loader, uint64_t entry_address,
    int argc, char** argv, char* stack, void* tls_block,
    size_t tls_block_size, bool trace_regs, bool trace_instructions) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->process = process;
  interpreter->guest_thread = guest_thread;
  interpreter->trace_regs = trace_regs;
  interpreter->trace_instructions = trace_instructions;
  interpreter->num_steps = -1;

  // stmdb sp!, {r7}; mov r7, #ARM_SYSCALL_RESOLVE; svc #0.  The host-side
  // resolver restores r7 and sp because svc transfers directly to the target.
  interpreter->symbol_resolver_code[0] = 0xe92d0080u;
  interpreter->symbol_resolver_code[1] =
      ARM_AL | (1u << 25) | (0xdu << 21) | (0u << 16) |
      (ARM_SYSCALL_REG << 12) | ARM_SYSCALL_RESOLVE;
  interpreter->symbol_resolver_code[2] = ARM_AL | 0x0f000000u;

  if (stack != NULL) {
    interpreter->stack = stack;
  } else {
    interpreter->stack = malloc(ARM_STACK_SIZE);
    interpreter->owns_stack = true;
  }
  uint32_t slot = guest_thread != NULL ? guest_thread->slot : 0;
  interpreter->stack_guest_base =
      ARM_STACK_BASE - slot * 0x01000000u;
  if (tls_block != NULL) {
    interpreter->tls_block = tls_block;
    interpreter->tls_block_size = tls_block_size;
    interpreter->tls_guest_base =
        ARM_TLS_BASE + slot * 0x00100000u;
    interpreter->tp_base = interpreter->tls_guest_base;
  } else if (loader->tls.present && loader->tls.main_thread_block != NULL) {
    interpreter->tls_block = loader->tls.main_thread_block;
    interpreter->tls_block_size = loader->tls.block_size;
    interpreter->tls_guest_base = ARM_TLS_BASE;
    interpreter->tp_base =
        interpreter->tls_guest_base +
        (uint32_t)(loader->tls.tp_base -
                   (uint64_t)(uintptr_t)loader->tls.main_thread_block);
  }
  interpreter->regs[ARM_SP_REG] = ARMInterpreterInitialSp(interpreter);
  interpreter->pc = entry_address;
  interpreter->running = entry_address != 0;
  interpreter->regs[ARM_LR_REG] = 0;
  SetupGuestMainArgs(interpreter, loader, argc, argv, entry_address,
                     loader->is_static);
  MapGuestResolver(interpreter);
}

void ARMInterpreterInit(ARMInterpreter* interpreter, Loader* loader,
                        uint64_t entry_address, int argc, char** argv,
                        bool trace_regs, bool trace_instructions) {
  ARMInterpreterInitForThread(interpreter, NULL, NULL, loader, entry_address,
                              argc, argv, NULL, NULL, 0, trace_regs,
                              trace_instructions);
}

void ARMInterpreterPrepareMain(ARMInterpreter* interpreter,
                               uint64_t entry_address, int argc, char** argv,
                               bool is_static_link) {
  interpreter->regs[ARM_SP_REG] = ARMInterpreterInitialSp(interpreter);
  interpreter->pc = entry_address;
  interpreter->regs[ARM_LR_REG] = 0;
  SetupGuestMainArgs(interpreter, interpreter->loader, argc, argv,
                     entry_address, is_static_link);
}

static void ARMInterpreterPrepareCall(ARMInterpreter* interpreter, uint64_t fn) {
  interpreter->regs[ARM_SP_REG] = ARMInterpreterInitialSp(interpreter);
  interpreter->pc = fn;
  interpreter->regs[ARM_LR_REG] = 0;
}

static int ARMInterpreterRunLoop(ARMInterpreter* interpreter) {
  for (;;) {
    if (interpreter->pc == 0) {
      return (int)(ReadReg(interpreter, 0) & 0xffffffffu);
    }
    if (interpreter->num_steps == 0) {
      return 0;
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
          printf("r%d: 0x%08" PRIx64 " -> 0x%08" PRIx64 "\n", i,
                 interpreter->old_regs[i], interpreter->regs[i]);
        }
      }
    }
  }
}

int ARMInterpreterCall(ARMInterpreter* interpreter, uint64_t fn) {
  ARMInterpreterPrepareCall(interpreter, fn);
  return ARMInterpreterRunLoop(interpreter);
}

int ARMInterpreterCallWithArg(ARMInterpreter* interpreter, uint64_t fn,
                              uint32_t arg) {
  ARMInterpreterPrepareCall(interpreter, fn);
  WriteReg(interpreter, 0, arg);
  return ARMInterpreterRunLoop(interpreter);
}

int ARMInterpreterRun(ARMInterpreter* interpreter) {
  return ARMInterpreterRunLoop(interpreter);
}

bool ARMGuestAddressExecutable(Loader* loader, uint64_t addr) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    if (region->segment->type != PT(load) ||
        (region->segment->flags & PF(x)) == 0) {
      continue;
    }
    uint64_t base = (uint64_t)(uintptr_t)region->address;
    uint64_t end = base + (uint64_t)region->length;
    if (addr >= base && addr < end) {
      return true;
    }
  }
  return false;
}

uint64_t ARMGuestFunctionRuntime(Loader* loader, uint64_t addr) {
  if (ARMGuestAddressExecutable(loader, addr)) {
    return addr;
  }
  uint64_t runtime = 0;
  if (LoaderLinkedAddressToRuntime(loader, NULL, addr, &runtime) &&
      ARMGuestAddressExecutable(loader, runtime)) {
    return runtime;
  }
  return 0;
}

uint64_t ARMLookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !ARMGuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

void ARMGuestCallVoidFunction(ARMInterpreter* cpu, uint64_t fn) {
  if (cpu == NULL || fn == 0) {
    return;
  }
  ARMInterpreterCall(cpu, fn);
}

void ARMGuestRunProgramFini(Loader* loader, ARMInterpreter* cpu) {
  ARMGuestCallVoidFunction(cpu, ARMLookupGuestFunction(loader, "__davecc_run_fini"));
}

bool ARMGuestRunProgramShutdown(Loader* loader, ARMInterpreter* cpu) {
  if (!LoaderLifecycleExecutableFiniAlreadyDone(loader->lifecycle)) {
    uint64_t guest_fini = ARMLookupGuestFunction(loader, "__davecc_run_fini");
    if (guest_fini != 0) {
      ARMGuestCallVoidFunction(cpu, guest_fini);
      LoaderLifecycleMarkExecutableFiniComplete(loader, loader->lifecycle);
    }
  }
  return ARMGuestRunFiniArrays(loader, cpu);
}

static bool ARMLifecycleCallback(void* context, LoadedDynamicLibrary* image,
                                 uint64_t function,
                                 LoaderLifecyclePhase phase) {
  (void)phase;
  typedef struct {
    Loader* loader;
    ARMInterpreter* cpu;
  } ARMLifecycleContext;
  ARMLifecycleContext* ctx = context;
  uint64_t call_addr = function;
  if (ctx->loader->arch->ignore_vaddr) {
    if (!LoaderLinkedAddressToRuntime(ctx->loader, image, function,
                                      &call_addr)) {
      LoaderError("Cannot translate function array entry 0x%llx\n",
                  (unsigned long long)function);
      return false;
    }
  }
  if (!ARMGuestAddressExecutable(ctx->loader, call_addr)) {
    LoaderError("Function array entry 0x%llx is not executable\n",
                (unsigned long long)call_addr);
    return false;
  }
  ARMGuestCallVoidFunction(ctx->cpu, call_addr);
  return true;
}

static bool RunGuestLifecyclePhase(Loader* loader, ARMInterpreter* cpu,
                                   LoaderLifecyclePhase phase) {
  typedef struct {
    Loader* loader;
    ARMInterpreter* cpu;
  } ARMLifecycleContext;
  ARMLifecycleContext ctx = {loader, cpu};
  return LoaderLifecycleRunPhase(loader, loader->lifecycle, phase,
                                 ARMLifecycleCallback, &ctx);
}

bool ARMGuestRunInitArrays(Loader* loader, ARMInterpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecyclePreinit) &&
         RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleInit);
}

bool ARMGuestRunFiniArrays(Loader* loader, ARMInterpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleFini);
}

int ARMGuestRunProgram(ARMInterpreter* interpreter, Loader* loader,
                       uint64_t entry_address, int argc, char** argv,
                       bool trace_regs, bool trace_instructions) {
  ARMProcessRuntime process;
  if (!ARMProcessRuntimeInit(&process, loader)) {
    return 1;
  }
  ARMInterpreterInitForThread(interpreter, &process, NULL, loader,
                              entry_address, argc, argv, NULL, NULL, 0,
                              trace_regs, trace_instructions);
  if (ARMProcessAttachMainThread(&process, interpreter) == NULL) {
    ARMProcessRuntimeDestruct(&process);
    return 1;
  }
  if (!ARMGuestRunInitArrays(loader, interpreter)) {
    ARMProcessRuntimeDestruct(&process);
    return 1;
  }
  ARMInterpreterPrepareMain(interpreter, entry_address, argc, argv,
                            loader->is_static);
  int result = ARMInterpreterRun(interpreter);
  if (!ARMGuestRunProgramShutdown(loader, interpreter)) {
    result = 1;
  }
  ARMProcessRuntimeDestruct(&process);
  return result;
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
          printf("r%d: 0x%08" PRIx64 " -> 0x%08" PRIx64 "\n", i,
                 interpreter->old_regs[i], interpreter->regs[i]);
        }
      }
    }
  }
}

void ARMInterpreterDestruct(ARMInterpreter* interpreter) {
  if (interpreter->owns_stack) {
    free(interpreter->stack);
  }
  interpreter->stack = NULL;
}
