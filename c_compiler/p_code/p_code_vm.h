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
#include "value_state.h"

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
  kPCodeVMStatusInvalidConstantOperation,
  kPCodeVMStatusUncaughtException,
} PCodeVMStatus;

typedef struct PCodeVM PCodeVM;

typedef struct {
  uint64_t start;
  size_t size;
  bool writable;
  unsigned char* states;
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
  // Lowest stack address holding a non-valid byte, or UINT64_MAX.
  uint64_t stack_ended_low;
  // The last failed read touched an object whose lifetime had ended.
  bool read_after_lifetime;
  // State of the first non-valid byte the last failed read touched.
  unsigned char failed_read_state;
};

// A memory-region byte state beyond ValueState: the byte belongs to an object
// whose lifetime has ended.  Reading it fails; writing it starts a new object.
#define PCODE_VM_STATE_ENDED_LIFETIME 3

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
bool PCodeVMRegisterStatefulMemoryRegion(PCodeVM* vm, void* memory, size_t size,
                                         bool writable,
                                         ValueState initial_state);
bool PCodeVMCopyMemoryState(PCodeVM* vm, uint64_t destination,
                            uint64_t source, size_t size);
bool PCodeVMEndLifetime(PCodeVM* vm, uint64_t address, size_t size);
// Gives a new object's bytes an indeterminate or erroneous value; writing a
// byte makes it valid.
bool PCodeVMSetMemoryState(PCodeVM* vm, uint64_t address, size_t size,
                           ValueState state);
// Called whenever the stack pointer has moved up past non-valid stack bytes.
void PCodeVMForgetEndedStackLifetimes(PCodeVM* vm);
bool PCodeVMUnregisterMemoryRegion(PCodeVM* vm, void* memory);
PCodeVMStatus PCodeVMStep(PCodeVM* vm);
PCodeVMStatus PCodeVMRun(PCodeVM* vm);
const char* PCodeVMStatusName(PCodeVMStatus status);

#endif /* p_code_vm_h */
