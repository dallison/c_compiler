//
//  p_code_vm.c
//  c_compiler
//

#include "p_code_vm.h"
#include <stdlib.h>
#include <string.h>

#define DEST(inst) (((inst) >> 16) & 0xff)
#define SRC1(inst) (((inst) >> 8) & 0xff)
#define SRC2(inst) ((inst) & 0xff)

bool PCodeVMRegisterMemoryRegion(PCodeVM* vm, void* memory, size_t size,
                                 bool writable);

static PCodeVMStatus DefaultEscape(PCodeVM* vm, int32_t code, void* data) {
  (void)vm;
  (void)data;
  switch (code) {
    case 4:
      return kPCodeVMStatusHalted;
    case 0:
      return kPCodeVMStatusUndefinedInstruction;
    case 1:
      return kPCodeVMStatusDivisionByZero;
    default:
      return kPCodeVMStatusUndefinedEscape;
  }
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

static bool RegionContains(PCodeVMMemoryRegion* region, uint64_t address,
                           size_t size, bool write) {
  if (write && !region->writable) {
    return false;
  }
  if (size == 0) {
    return address >= region->start &&
           address <= region->start + region->size;
  }
  if (size > UINT64_MAX - region->start ||
      address < region->start || size > UINT64_MAX - address) {
    return false;
  }
  uint64_t end = address + size;
  uint64_t region_end = region->start + region->size;
  return end <= region_end;
}

static bool CheckMemoryAccess(PCodeVM* vm, uint64_t address, size_t size,
                              bool write) {
  if (!vm->checked_memory) {
    return true;
  }
  if (vm->stack != NULL && vm->stack_size != 0) {
    uint64_t stack_start = (uint64_t)(uintptr_t)vm->stack;
    uint64_t stack_end = stack_start + vm->stack_size;
    if (address >= stack_start && address < stack_end) {
      if (size > UINT64_MAX - address) {
        vm->status =
            write ? kPCodeVMStatusInvalidWrite : kPCodeVMStatusInvalidRead;
        return false;
      }
      uint64_t end = address + size;
      if (address >= (uint64_t)vm->iregs[PCODE_SP_REG] && end <= stack_end) {
        return true;
      }
      vm->status =
          write ? kPCodeVMStatusInvalidWrite : kPCodeVMStatusInvalidRead;
      return false;
    }
  }
  for (size_t i = 0; i < vm->memory_region_count; i++) {
    if (RegionContains(&vm->memory_regions[i], address, size, write)) {
      return true;
    }
  }
  vm->status =
      write ? kPCodeVMStatusInvalidWrite : kPCodeVMStatusInvalidRead;
  return false;
}

static bool NormalizeCheckedAddress(PCodeVM* vm, uint64_t raw, size_t size,
                                    bool write, uint64_t* normalized) {
  if (!vm->checked_memory) {
    *normalized = raw;
    return true;
  }
  if (vm->stack != NULL && vm->stack_size != 0) {
    uint64_t stack_start = (uint64_t)(uintptr_t)vm->stack;
    uint64_t stack_end = stack_start + vm->stack_size;
    if (raw >= stack_start && raw + size <= stack_end) {
      if (raw >= (uint64_t)vm->iregs[PCODE_SP_REG]) {
        *normalized = raw;
        return true;
      }
    }
    uint64_t stack_address =
        (stack_start & ~UINT64_C(0xffffffff)) | (raw & UINT64_C(0xffffffff));
    if (stack_address + size <= stack_end &&
        stack_address >= (uint64_t)vm->iregs[PCODE_SP_REG]) {
      *normalized = stack_address;
      return true;
    }
  }
  for (size_t i = vm->memory_region_count; i > 0; --i) {
    PCodeVMMemoryRegion* region = &vm->memory_regions[i - 1];
    uint64_t region_address =
        (region->start & ~UINT64_C(0xffffffff)) | (raw & UINT64_C(0xffffffff));
    if (RegionContains(region, region_address, size, write)) {
      *normalized = region_address;
      return true;
    }
  }
  *normalized = raw;
  return false;
}

static void* AccessPointer(PCodeVM* vm, uint64_t address, size_t size,
                           bool write) {
  uint64_t normalized = address;
  if (!CheckMemoryAccess(vm, normalized, size, write)) {
    if (!NormalizeCheckedAddress(vm, address, size, write, &normalized) ||
        !CheckMemoryAccess(vm, normalized, size, write)) {
      return NULL;
    }
    vm->status = kPCodeVMStatusRunning;
  }
  return (void*)(uintptr_t)normalized;
}

static bool ReadVMU8(PCodeVM* vm, uint64_t address, uint8_t* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = *(uint8_t*)p;
  return true;
}

static bool ReadVMU16(PCodeVM* vm, uint64_t address, uint16_t* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = ReadU16(p);
  return true;
}

static bool ReadVMU32(PCodeVM* vm, uint64_t address, uint32_t* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = ReadU32(p);
  return true;
}

static bool ReadVMU64(PCodeVM* vm, uint64_t address, uint64_t* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = ReadU64(p);
  return true;
}

static bool ReadVMFloat(PCodeVM* vm, uint64_t address, float* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = ReadFloat(p);
  return true;
}

static bool ReadVMDouble(PCodeVM* vm, uint64_t address, double* value) {
  void* p = AccessPointer(vm, address, sizeof(*value), false);
  if (p == NULL) {
    return false;
  }
  *value = ReadDouble(p);
  return true;
}

static bool WriteVMU8(PCodeVM* vm, uint64_t address, uint8_t value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  *(uint8_t*)p = value;
  return true;
}

static bool WriteVMU16(PCodeVM* vm, uint64_t address, uint16_t value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  WriteU16(p, value);
  return true;
}

static bool WriteVMU32(PCodeVM* vm, uint64_t address, uint32_t value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  WriteU32(p, value);
  return true;
}

static bool WriteVMU64(PCodeVM* vm, uint64_t address, uint64_t value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  WriteU64(p, value);
  return true;
}

static bool WriteVMFloat(PCodeVM* vm, uint64_t address, float value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  WriteFloat(p, value);
  return true;
}

static bool WriteVMDouble(PCodeVM* vm, uint64_t address, double value) {
  void* p = AccessPointer(vm, address, sizeof(value), true);
  if (p == NULL) {
    return false;
  }
  WriteDouble(p, value);
  return true;
}

void PCodeVMInit(PCodeVM* vm) {
  memset(vm, 0, sizeof(PCodeVM));
  vm->max_steps = -1;
  vm->status = kPCodeVMStatusRunning;
  vm->escape = DefaultEscape;
}

bool PCodeVMInitWithStack(PCodeVM* vm, size_t stack_size) {
  PCodeVMInit(vm);
  vm->stack = malloc(stack_size);
  if (vm->stack == NULL) {
    return false;
  }
  vm->stack_size = stack_size;
  vm->owns_stack = true;
  vm->iregs[PCODE_SP_REG] = (int64_t)(vm->stack + vm->stack_size);
  return true;
}

void PCodeVMDestruct(PCodeVM* vm) {
  if (vm->owns_stack) {
    free(vm->stack);
  }
  free(vm->memory_regions);
  vm->stack = NULL;
  vm->stack_size = 0;
  vm->owns_stack = false;
  vm->memory_regions = NULL;
  vm->memory_region_count = 0;
  vm->memory_region_capacity = 0;
  vm->checked_memory = false;
}

void PCodeVMSetStack(PCodeVM* vm, void* stack, size_t stack_size) {
  if (vm->owns_stack) {
    free(vm->stack);
  }
  vm->stack = stack;
  vm->stack_size = stack_size;
  vm->owns_stack = false;
  vm->iregs[PCODE_SP_REG] = (int64_t)((char*)stack + stack_size);
}

void PCodeVMSetEntry(PCodeVM* vm, uint64_t entry_address) {
  vm->iregs[PCODE_PC_REG] = (int64_t)entry_address;
  vm->status = kPCodeVMStatusRunning;
}

void PCodeVMSetEscapeHandler(PCodeVM* vm, PCodeVMEscapeHandler handler,
                             void* data) {
  vm->escape = handler != NULL ? handler : DefaultEscape;
  vm->escape_data = data;
}

bool PCodeVMEnableCheckedMemory(PCodeVM* vm) {
  vm->checked_memory = true;
  if (vm->stack != NULL && vm->stack_size != 0) {
    return PCodeVMRegisterMemoryRegion(vm, vm->stack, vm->stack_size, true);
  }
  return true;
}

bool PCodeVMRegisterMemoryRegion(PCodeVM* vm, void* memory, size_t size,
                                 bool writable) {
  if (memory == NULL || size == 0) {
    return true;
  }
  if (vm->memory_region_count == vm->memory_region_capacity) {
    size_t new_capacity =
        vm->memory_region_capacity == 0 ? 8 : vm->memory_region_capacity * 2;
    PCodeVMMemoryRegion* new_regions =
        realloc(vm->memory_regions, new_capacity * sizeof(*new_regions));
    if (new_regions == NULL) {
      return false;
    }
    vm->memory_regions = new_regions;
    vm->memory_region_capacity = new_capacity;
  }
  vm->memory_regions[vm->memory_region_count++] = (PCodeVMMemoryRegion){
      .start = (uint64_t)(uintptr_t)memory,
      .size = size,
      .writable = writable,
  };
  return true;
}

bool PCodeVMUnregisterMemoryRegion(PCodeVM* vm, void* memory) {
  uint64_t start = (uint64_t)(uintptr_t)memory;
  for (size_t i = 0; i < vm->memory_region_count; i++) {
    if (vm->memory_regions[i].start == start) {
      vm->memory_regions[i] = vm->memory_regions[vm->memory_region_count - 1];
      vm->memory_region_count--;
      return true;
    }
  }
  return false;
}

static PCodeVMStatus HandleEscape(PCodeVM* vm, int32_t code) {
  if (vm->escape == NULL) {
    return DefaultEscape(vm, code, NULL);
  }
  return vm->escape(vm, code, vm->escape_data);
}

static PCodeVMStatus DivisionByZero(PCodeVM* vm) {
  return HandleEscape(vm, 1);
}

static PCodeVMStatus UndefinedInstruction(PCodeVM* vm) {
  return HandleEscape(vm, 0);
}

PCodeVMStatus PCodeVMStep(PCodeVM* vm) {
  if (vm->status != kPCodeVMStatusRunning) {
    return vm->status;
  }
  if (vm->max_steps >= 0 && vm->steps >= vm->max_steps) {
    vm->status = kPCodeVMStatusStepLimit;
    return vm->status;
  }
  vm->steps++;

  int64_t* iregs = vm->iregs;
  float* fregs = vm->fregs;
  double* dregs = vm->dregs;
  uint64_t pc = (uint64_t)iregs[PCODE_PC_REG];
  uint32_t inst;
  if (!ReadVMU32(vm, pc, &inst)) {
    return vm->status;
  }
  pc += 4;
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
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] = iregs[SRC1(inst)] / iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divu):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] =
              (uint64_t)iregs[SRC1(inst)] / (uint64_t)iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divf):
        if (fregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          fregs[DEST(inst)] = fregs[SRC1(inst)] / fregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divd):
        if (dregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          dregs[DEST(inst)] = dregs[SRC1(inst)] / dregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(mod):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] = iregs[SRC1(inst)] % iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(modu):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] =
              (uint64_t)iregs[SRC1(inst)] % (uint64_t)iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(lsr):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] >> iregs[SRC2(inst)];
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
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] < (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpleu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] <= (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgtu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] > (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgeu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] >= (uint64_t)iregs[SRC2(inst)];
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
      case PCODE_OP(cmp3way):
        iregs[DEST(inst)] = (iregs[SRC1(inst)] < iregs[SRC2(inst)])   ? -1
                            : (iregs[SRC1(inst)] > iregs[SRC2(inst)]) ? 1
                                                                      : 0;
        break;
      case PCODE_OP(cmp3wayu):
        iregs[DEST(inst)] =
            ((uint64_t)iregs[SRC1(inst)] < (uint64_t)iregs[SRC2(inst)])   ? -1
            : ((uint64_t)iregs[SRC1(inst)] > (uint64_t)iregs[SRC2(inst)]) ? 1
                                                                         : 0;
        break;
      case PCODE_OP(cmp3wayf):
        iregs[DEST(inst)] = (fregs[SRC1(inst)] < fregs[SRC2(inst)])    ? -1
                            : (fregs[SRC1(inst)] > fregs[SRC2(inst)])  ? 1
                            : (fregs[SRC1(inst)] == fregs[SRC2(inst)]) ? 0
                                                                       : 2;
        break;
      case PCODE_OP(cmp3wayd):
        iregs[DEST(inst)] = (dregs[SRC1(inst)] < dregs[SRC2(inst)])    ? -1
                            : (dregs[SRC1(inst)] > dregs[SRC2(inst)])  ? 1
                            : (dregs[SRC1(inst)] == dregs[SRC2(inst)]) ? 0
                                                                       : 2;
        break;
      case PCODE_OP(decsp):
        iregs[PCODE_SP_REG] -= inst & 0xffffff;
        break;
      case PCODE_OP(incsp):
        iregs[PCODE_SP_REG] += inst & 0xffffff;
        break;
      case PCODE_OP(push):
        iregs[PCODE_SP_REG] -= 4;
        if (!WriteVMU32(vm, (uint64_t)iregs[PCODE_SP_REG],
                        (uint32_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        break;
      case PCODE_OP(pushf):
        iregs[PCODE_SP_REG] -= 4;
        if (!WriteVMFloat(vm, (uint64_t)iregs[PCODE_SP_REG],
                          fregs[DEST(inst)])) {
          return vm->status;
        }
        break;
      case PCODE_OP(pushd):
        iregs[PCODE_SP_REG] -= 8;
        if (!WriteVMDouble(vm, (uint64_t)iregs[PCODE_SP_REG],
                           dregs[DEST(inst)])) {
          return vm->status;
        }
        break;
      case PCODE_OP(pushx):
        iregs[PCODE_SP_REG] -= 8;
        if (!WriteVMU64(vm, (uint64_t)iregs[PCODE_SP_REG],
                        (uint64_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        break;
      case PCODE_OP(pop): {
        uint32_t value;
        if (!ReadVMU32(vm, (uint64_t)iregs[PCODE_SP_REG], &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = (int32_t)value;
        iregs[PCODE_SP_REG] += 4;
        break;
      }
      case PCODE_OP(popf): {
        float value;
        if (!ReadVMFloat(vm, (uint64_t)iregs[PCODE_SP_REG], &value)) {
          return vm->status;
        }
        fregs[DEST(inst)] = value;
        iregs[PCODE_SP_REG] += 4;
        break;
      }
      case PCODE_OP(popd): {
        double value;
        if (!ReadVMDouble(vm, (uint64_t)iregs[PCODE_SP_REG], &value)) {
          return vm->status;
        }
        dregs[DEST(inst)] = value;
        iregs[PCODE_SP_REG] += 8;
        break;
      }
      case PCODE_OP(popx): {
        uint64_t value;
        if (!ReadVMU64(vm, (uint64_t)iregs[PCODE_SP_REG], &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = value;
        iregs[PCODE_SP_REG] += 8;
        break;
      }
      case PCODE_OP(mov):
        iregs[DEST(inst)] = iregs[SRC1(inst)];
        break;
      case PCODE_OP(movf):
        fregs[DEST(inst)] = fregs[SRC1(inst)];
        break;
      case PCODE_OP(movd):
        dregs[DEST(inst)] = dregs[SRC1(inst)];
        break;
      case PCODE_OP(ret): {
        uint64_t return_address;
        if (!ReadVMU64(vm, (uint64_t)iregs[PCODE_SP_REG], &return_address)) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] = return_address;
        iregs[PCODE_SP_REG] += 8;
        break;
      }
      case PCODE_OP(cbra):
        if (iregs[DEST(inst)] != 0) {
          iregs[PCODE_PC_REG] = iregs[SRC1(inst)];
        }
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
        if (!WriteVMU64(vm, (uint64_t)iregs[PCODE_SP_REG],
                        (uint64_t)iregs[PCODE_PC_REG])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] = iregs[DEST(inst)];
        break;
      case PCODE_OP(esc): {
        PCodeVMStatus status = HandleEscape(vm, inst & 0xffffff);
        if (status != kPCodeVMStatusRunning) {
          vm->status = status;
        }
        break;
      }
      default:
        vm->status = UndefinedInstruction(vm);
        break;
    }
    return vm->status;
  }

  bool is_64_bit = (inst & 0x40000000) == 0;
  if (is_64_bit) {
    switch ((inst >> 24) & 0x3f) {
      case PCODE_OP(ldw): {
        uint32_t offset;
        uint32_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU32(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = (int32_t)value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldh): {
        uint32_t offset;
        uint16_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU16(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = (int16_t)value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldb): {
        uint32_t offset;
        uint8_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU8(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                      &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = (int8_t)value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(lduw): {
        uint32_t offset;
        uint32_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU32(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldub): {
        uint32_t offset;
        uint8_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU8(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                      &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(lduh): {
        uint32_t offset;
        uint16_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU16(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldx): {
        uint32_t offset;
        uint64_t value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMU64(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldf): {
        uint32_t offset;
        float value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMFloat(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                         &value)) {
          return vm->status;
        }
        fregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(ldd): {
        uint32_t offset;
        double value;
        if (!ReadVMU32(vm, pc, &offset) ||
            !ReadVMDouble(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                          &value)) {
          return vm->status;
        }
        dregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(stw): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMU32(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                        (uint32_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(sth): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMU16(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                        (uint16_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(stx): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMU64(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                        (uint64_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(stf): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMFloat(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                          fregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(std): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMDouble(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                           dregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(stb): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset) ||
            !WriteVMU8(vm, (uint64_t)iregs[SRC1(inst)] + (int32_t)offset,
                       (uint8_t)iregs[DEST(inst)])) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(movc): {
        uint32_t value;
        if (!ReadVMU32(vm, pc, &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = (int32_t)value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(movfc): {
        float value;
        if (!ReadVMFloat(vm, pc, &value)) {
          return vm->status;
        }
        fregs[DEST(inst)] = value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      case PCODE_OP(bz): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset)) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] +=
            iregs[DEST(inst)] == 0 ? (int32_t)offset + 4 : 4;
        break;
      }
      case PCODE_OP(bnz): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset)) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] +=
            iregs[DEST(inst)] != 0 ? (int32_t)offset + 4 : 4;
        break;
      }
      case PCODE_OP(bra): {
        uint32_t offset;
        if (!ReadVMU32(vm, pc, &offset)) {
          return vm->status;
        }
        iregs[PCODE_PC_REG] += (int32_t)offset + 4;
        break;
      }
      case PCODE_OP(addc): {
        uint32_t value;
        if (!ReadVMU32(vm, pc, &value)) {
          return vm->status;
        }
        iregs[DEST(inst)] = iregs[SRC1(inst)] + (int32_t)value;
        iregs[PCODE_PC_REG] += 4;
        break;
      }
      default:
        vm->status = UndefinedInstruction(vm);
        break;
    }
    return vm->status;
  }

  switch ((inst >> 24) & 0x3f) {
    case PCODE_OP(movdc): {
      double value;
      if (!ReadVMDouble(vm, pc, &value)) {
        return vm->status;
      }
      dregs[DEST(inst)] = value;
      iregs[PCODE_PC_REG] += 8;
      break;
    }
    case PCODE_OP(movxc): {
      uint64_t value;
      if (!ReadVMU64(vm, pc, &value)) {
        return vm->status;
      }
      iregs[DEST(inst)] = value;
      iregs[PCODE_PC_REG] += 8;
      break;
    }
    case PCODE_OP(jmp): {
      uint64_t offset;
      if (!ReadVMU64(vm, pc, &offset)) {
        return vm->status;
      }
      iregs[PCODE_PC_REG] = offset + iregs[PCODE_PC_REG] + 8;
      break;
    }
    case PCODE_OP(call): {
      iregs[PCODE_SP_REG] -= 8;
      if (!WriteVMU64(vm, (uint64_t)iregs[PCODE_SP_REG],
                      (uint64_t)(iregs[PCODE_PC_REG] + 8))) {
        return vm->status;
      }
      uint64_t offset;
      if (!ReadVMU64(vm, pc, &offset)) {
        return vm->status;
      }
      iregs[PCODE_PC_REG] = offset + iregs[PCODE_PC_REG] + 8;
      break;
    }
    case PCODE_OP(cjmp): {
      uint64_t offset;
      uint64_t target;
      if (!ReadVMU64(vm, pc, &offset) ||
          !ReadVMU64(vm, (uint64_t)iregs[PCODE_PC_REG] + 8 + offset,
                     &target)) {
        return vm->status;
      }
      iregs[PCODE_PC_REG] = target;
      break;
    }
    case PCODE_OP(adr): {
      uint64_t offset;
      if (!ReadVMU64(vm, pc, &offset)) {
        return vm->status;
      }
      iregs[DEST(inst)] = offset + iregs[PCODE_PC_REG] + 8;
      iregs[PCODE_PC_REG] += 8;
      break;
    }
    default:
      vm->status = UndefinedInstruction(vm);
      break;
  }
  return vm->status;
}

PCodeVMStatus PCodeVMRun(PCodeVM* vm) {
  while (vm->status == kPCodeVMStatusRunning) {
    PCodeVMStep(vm);
  }
  return vm->status;
}

const char* PCodeVMStatusName(PCodeVMStatus status) {
  switch (status) {
    case kPCodeVMStatusRunning:
      return "running";
    case kPCodeVMStatusHalted:
      return "halted";
    case kPCodeVMStatusStepLimit:
      return "step limit";
    case kPCodeVMStatusUndefinedInstruction:
      return "undefined instruction";
    case kPCodeVMStatusDivisionByZero:
      return "division by zero";
    case kPCodeVMStatusUndefinedEscape:
      return "undefined escape";
    case kPCodeVMStatusInvalidRead:
      return "invalid read";
    case kPCodeVMStatusInvalidWrite:
      return "invalid write";
    case kPCodeVMStatusInvalidFree:
      return "invalid free";
    case kPCodeVMStatusAllocationFailure:
      return "allocation failure";
    case kPCodeVMStatusInvalidConstantOperation:
      return "invalid constant operation";
    case kPCodeVMStatusUncaughtException:
      return "uncaught constexpr exception";
  }
  return "unknown";
}
