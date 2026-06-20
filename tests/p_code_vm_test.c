#include <stdint.h>

#include "p_code_reg_alloc.h"
#include "p_code_vm.h"

static PCodeVMStatus TestEscape(PCodeVM* vm, int32_t code, void* data) {
  (void)vm;
  (void)data;
  return code == 4 ? kPCodeVMStatusHalted : kPCodeVMStatusUndefinedEscape;
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

int main(void) {
  int result = TestIntegerProgram();
  if (result != 0) {
    return result;
  }
  result = TestDoubleProgram();
  if (result != 0) {
    return result;
  }
  return TestStepLimit();
}
