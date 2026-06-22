//
//  p_code_vm.h
//  c_compiler
//

#ifndef p_code_vm_h
#define p_code_vm_h

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "p_code_machine.h"

#define P_CODE_VM_DEFAULT_STACK_SIZE (8 * 1024 * 1024)

typedef enum {
  kPCodeVMStatusRunning,
  kPCodeVMStatusHalted,
  kPCodeVMStatusStepLimit,
  kPCodeVMStatusUndefinedInstruction,
  kPCodeVMStatusDivisionByZero,
  kPCodeVMStatusUndefinedEscape,
  kPCodeVMStatusInvalidRead,
  kPCodeVMStatusInvalidWrite,
  kPCodeVMStatusInvalidFree,
  kPCodeVMStatusAllocationFailure,
} PCodeVMStatus;

typedef struct PCodeVM PCodeVM;

typedef struct {
  uint64_t start;
  size_t size;
  bool writable;
} PCodeVMMemoryRegion;

typedef PCodeVMStatus (*PCodeVMEscapeHandler)(PCodeVM* vm, int32_t code,
                                              void* data);

struct PCodeVM {
  int64_t iregs[PCODE_NUM_INT_REGS];
  float fregs[PCODE_NUM_FLOAT_REGS];
  double dregs[PCODE_NUM_DOUBLE_REGS];

  char* stack;
  size_t stack_size;
  bool owns_stack;

  int64_t steps;
  int64_t max_steps;
  PCodeVMStatus status;
  PCodeVMEscapeHandler escape;
  void* escape_data;

  bool checked_memory;
  PCodeVMMemoryRegion* memory_regions;
  size_t memory_region_count;
  size_t memory_region_capacity;
};

void PCodeVMInit(PCodeVM* vm);
bool PCodeVMInitWithStack(PCodeVM* vm, size_t stack_size);
void PCodeVMDestruct(PCodeVM* vm);

void PCodeVMSetStack(PCodeVM* vm, void* stack, size_t stack_size);
void PCodeVMSetEntry(PCodeVM* vm, uint64_t entry_address);
void PCodeVMSetEscapeHandler(PCodeVM* vm, PCodeVMEscapeHandler handler,
                             void* data);
bool PCodeVMEnableCheckedMemory(PCodeVM* vm);
bool PCodeVMRegisterMemoryRegion(PCodeVM* vm, void* memory, size_t size,
                                 bool writable);
bool PCodeVMUnregisterMemoryRegion(PCodeVM* vm, void* memory);
PCodeVMStatus PCodeVMStep(PCodeVM* vm);
PCodeVMStatus PCodeVMRun(PCodeVM* vm);
const char* PCodeVMStatusName(PCodeVMStatus status);

#endif /* p_code_vm_h */
