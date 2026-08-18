#include <stdint.h>

#include "p_code_encoding.h"
#include "p_code_reg_alloc.h"
#include "p_code_vm.h"

static PCodeVMStatus TestEscape(PCodeVM* vm, int32_t code, void* data) {
  (void)vm;
  (void)data;
  return code == 4 ? kPCodeVMStatusHalted : kPCodeVMStatusUndefinedEscape;
}

static int TestInstructionEncoding(void) {
  if (PCodeEncodeRegisters32(PCODE_OP(add), 3, 4, 5) !=
          ((PCODE_OP(add) << 24) | (3 << 16) | (4 << 8) | 5) ||
      PCodeEncodeRegisters64(PCODE_OP(ldx), 6, 7) !=
          (UINT32_C(0x80000000) | (PCODE_OP(ldx) << 24) | (6 << 16) |
           (7 << 8)) ||
      PCodeEncodeRegister96(PCODE_OP(movxc), 8) !=
          (UINT32_C(0xc0000000) | (PCODE_OP(movxc) << 24) | (8 << 16)) ||
      PCodeEncodeImmediate24(PCODE_OP(esc), -1) !=
          ((PCODE_OP(esc) << 24) | UINT32_C(0x00ffffff))) {
    return 13;
  }
  return 0;
}

static int TestIntegerProgram(void) {
  uint32_t program[] = {
      0xc0000000 | (PCODE_OP(movxc) << 24) | (PCODE_INT_RETURN_REG << 16),
      42,
      0,
      (PCODE_OP(esc) << 24) | 4,
  };
  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 1;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  PCodeVMSetEntry(&vm, (uint64_t)program);
  vm.max_steps = 8;
  PCodeVMStatus status = PCodeVMRun(&vm);
  int ok = status == kPCodeVMStatusHalted &&
           vm.iregs[PCODE_INT_RETURN_REG] == 42;
  PCodeVMDestruct(&vm);
  return ok ? 0 : 2;
}

static int TestDoubleProgram(void) {
  union {
    double f;
    uint64_t u;
  } value;
  value.f = 3.5;
  uint32_t program[] = {
      0xc0000000 | (PCODE_OP(movdc) << 24) | (PCODE_DOUBLE_RETURN_REG << 16),
      (uint32_t)(value.u & 0xffffffffu),
      (uint32_t)(value.u >> 32),
      (PCODE_OP(esc) << 24) | 4,
  };
  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 3;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  PCodeVMSetEntry(&vm, (uint64_t)program);
  vm.max_steps = 8;
  PCodeVMStatus status = PCodeVMRun(&vm);
  int ok = status == kPCodeVMStatusHalted &&
           vm.dregs[PCODE_DOUBLE_RETURN_REG] == 3.5;
  PCodeVMDestruct(&vm);
  return ok ? 0 : 4;
}

static int TestStepLimit(void) {
  uint32_t program[] = {
      (PCODE_OP(bra) << 24) | 0x80000000,
      (uint32_t)-8,
  };
  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 5;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  PCodeVMSetEntry(&vm, (uint64_t)program);
  vm.max_steps = 4;
  PCodeVMStatus status = PCodeVMRun(&vm);
  PCodeVMDestruct(&vm);
  return status == kPCodeVMStatusStepLimit ? 0 : 6;
}

static int TestCheckedMemory(void) {
  uint32_t slot = 0;
  uint32_t program[] = {
      0xc0000000 | (PCODE_OP(movxc) << 24) | (1 << 16),
      (uint32_t)((uint64_t)(uintptr_t)&slot & 0xffffffffu),
      (uint32_t)((uint64_t)(uintptr_t)&slot >> 32),
      0x80000000 | (PCODE_OP(movc) << 24) | (2 << 16),
      123,
      0x80000000 | (PCODE_OP(stw) << 24) | (2 << 16) | (1 << 8),
      0,
      (PCODE_OP(esc) << 24) | 4,
  };
  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 7;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  if (!PCodeVMEnableCheckedMemory(&vm) ||
      !PCodeVMRegisterMemoryRegion(&vm, program, sizeof(program), false) ||
      !PCodeVMRegisterMemoryRegion(&vm, &slot, sizeof(slot), true)) {
    PCodeVMDestruct(&vm);
    return 8;
  }
  PCodeVMSetEntry(&vm, (uint64_t)(uintptr_t)program);
  vm.max_steps = 8;
  PCodeVMStatus status = PCodeVMRun(&vm);
  int ok = status == kPCodeVMStatusHalted && slot == 123;
  PCodeVMDestruct(&vm);
  if (!ok) {
    return 9;
  }

  slot = 0;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 10;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  if (!PCodeVMEnableCheckedMemory(&vm) ||
      !PCodeVMRegisterMemoryRegion(&vm, program, sizeof(program), false)) {
    PCodeVMDestruct(&vm);
    return 11;
  }
  PCodeVMSetEntry(&vm, (uint64_t)(uintptr_t)program);
  vm.max_steps = 8;
  status = PCodeVMRun(&vm);
  PCodeVMDestruct(&vm);
  return status == kPCodeVMStatusInvalidWrite ? 0 : 12;
}

static int TestValueStateMemory(void) {
  uint32_t slot = 0;
  uint32_t read_program[] = {
      0xc0000000 | (PCODE_OP(movxc) << 24) | (1 << 16),
      (uint32_t)((uint64_t)(uintptr_t)&slot & 0xffffffffu),
      (uint32_t)((uint64_t)(uintptr_t)&slot >> 32),
      0x80000000 | (PCODE_OP(ldw) << 24) |
          (PCODE_INT_RETURN_REG << 16) | (1 << 8),
      0,
      (PCODE_OP(esc) << 24) | 4,
  };
  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 14;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  if (!PCodeVMEnableCheckedMemory(&vm) ||
      !PCodeVMRegisterMemoryRegion(
          &vm, read_program, sizeof(read_program), false) ||
      !PCodeVMRegisterStatefulMemoryRegion(
          &vm, &slot, sizeof(slot), true, kValueStateErroneous)) {
    PCodeVMDestruct(&vm);
    return 15;
  }
  PCodeVMSetEntry(&vm, (uint64_t)(uintptr_t)read_program);
  vm.max_steps = 8;
  PCodeVMStatus status = PCodeVMRun(&vm);
  PCodeVMDestruct(&vm);
  if (status != kPCodeVMStatusInvalidRead) {
    return 16;
  }

  uint32_t write_then_read_program[] = {
      0xc0000000 | (PCODE_OP(movxc) << 24) | (1 << 16),
      (uint32_t)((uint64_t)(uintptr_t)&slot & 0xffffffffu),
      (uint32_t)((uint64_t)(uintptr_t)&slot >> 32),
      0x80000000 | (PCODE_OP(movc) << 24) | (2 << 16),
      123,
      0x80000000 | (PCODE_OP(stw) << 24) | (2 << 16) | (1 << 8),
      0,
      0x80000000 | (PCODE_OP(ldw) << 24) |
          (PCODE_INT_RETURN_REG << 16) | (1 << 8),
      0,
      (PCODE_OP(esc) << 24) | 4,
  };
  if (!PCodeVMInitWithStack(&vm, 4096)) {
    return 17;
  }
  PCodeVMSetEscapeHandler(&vm, TestEscape, 0);
  if (!PCodeVMEnableCheckedMemory(&vm) ||
      !PCodeVMRegisterMemoryRegion(
          &vm, write_then_read_program, sizeof(write_then_read_program),
          false) ||
      !PCodeVMRegisterStatefulMemoryRegion(
          &vm, &slot, sizeof(slot), true, kValueStateIndeterminate)) {
    PCodeVMDestruct(&vm);
    return 18;
  }
  PCodeVMSetEntry(&vm, (uint64_t)(uintptr_t)write_then_read_program);
  vm.max_steps = 12;
  status = PCodeVMRun(&vm);
  int ok = status == kPCodeVMStatusHalted && slot == 123 &&
           vm.iregs[PCODE_INT_RETURN_REG] == 123;
  PCodeVMDestruct(&vm);
  return ok ? 0 : 19;
}

int main(void) {
  int result = TestInstructionEncoding();
  if (result != 0) {
    return result;
  }
  result = TestIntegerProgram();
  if (result != 0) {
    return result;
  }
  result = TestDoubleProgram();
  if (result != 0) {
    return result;
  }
  result = TestStepLimit();
  if (result != 0) {
    return result;
  }
  result = TestCheckedMemory();
  if (result != 0) {
    return result;
  }
  return TestValueStateMemory();
}
