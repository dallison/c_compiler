//
//  arm_interpreter.h
//  arm_interpreter
//

#ifndef arm_interpreter_h
#define arm_interpreter_h

#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_machine.h"
#include "loader.h"

#define ARM_STACK_SIZE (8 * 1024 * 1024)
// The fixed 32-bit guest map has sixteen non-overlapping 16 MiB stack windows
// between the TLS and main-stack regions.
#define ARM_MAX_GUEST_THREADS 16
// Guest virtual address at which the interpreter maps the stack.  Chosen to sit
// above the code/data/heap (which start at 0x40000000) and below 0x80000000.
#define ARM_STACK_BASE 0x70000000u
#define ARM_TLS_BASE 0x60000000u
#define ARM_BREAKPOINT_INSN 0xE7F001F0u

#define ARM_SYSCALL_REG 7

#define ARM_SYSCALL_HALT 1
#define ARM_SYSCALL_OPEN 2
#define ARM_SYSCALL_CLOSE 3
#define ARM_SYSCALL_WRITE 4
#define ARM_SYSCALL_READ 5
#define ARM_SYSCALL_RESOLVE 6
#define ARM_SYSCALL_LSEEK 7
#define ARM_SYSCALL_MALLOC 8
#define ARM_SYSCALL_FREE 9
#define ARM_SYSCALL_REALLOC 10
#define ARM_SYSCALL_ABORT 11
#define ARM_SYSCALL_EXIT 12
#define ARM_SYSCALL_TIME 13
#define ARM_SYSCALL_CLOCK 14
#define ARM_SYSCALL_THREAD_CREATE 15
#define ARM_SYSCALL_THREAD_JOIN 16
#define ARM_SYSCALL_THREAD_SELF 17
#define ARM_SYSCALL_GET_TP 18
#define ARM_SYSCALL_THREAD_EXIT 19
#define ARM_SYSCALL_HEAP_LOCK 20
#define ARM_SYSCALL_HEAP_UNLOCK 21
#define ARM_SYSCALL_EXIT_CLEAN 22

struct ARMProcessRuntime;
struct ARMGuestThread;

typedef struct ARMInterpreter {
  Loader* loader;
  struct ARMProcessRuntime* process;
  struct ARMGuestThread* guest_thread;
  uint64_t regs[ARM_NUM_INT_REGS];
  uint32_t cpsr;
  float sregs[ARM_NUM_FLOAT_REGS];
  uint64_t old_regs[ARM_NUM_INT_REGS];
  float old_sregs[ARM_NUM_FLOAT_REGS];
  uint32_t symbol_resolver_code[2];
  char* stack;
  uint32_t stack_guest_base;
  bool owns_stack;
  void* tls_block;
  uint32_t tls_guest_base;
  uint64_t pc;
  SymbolScope* current_symbol;
  bool trace_regs;
  bool trace_instructions;
  int num_steps;
  bool reservation_valid;
  uint64_t reservation_address;
  size_t reservation_size;
  uint64_t reservation_epoch;
  uint32_t tp_base;
  size_t tls_block_size;
  bool running;
  int exit_code;
  jmp_buf debugger;
} ARMInterpreter;

void ARMInterpreterInit(ARMInterpreter* interpreter, Loader* loader,
                        uint64_t entry_address, int argc, char** argv,
                        bool trace_regs, bool trace_instructions);
void ARMInterpreterInitForThread(
    ARMInterpreter* interpreter, struct ARMProcessRuntime* process,
    struct ARMGuestThread* guest_thread, Loader* loader,
    uint64_t entry_address, int argc, char** argv, char* stack,
    void* tls_block, size_t tls_block_size, bool trace_regs,
    bool trace_instructions);
void ARMInterpreterPrepareMain(ARMInterpreter* interpreter,
                               uint64_t entry_address, int argc, char** argv,
                               bool is_static_link);
int ARMInterpreterCall(ARMInterpreter* interpreter, uint64_t fn);
int ARMInterpreterCallWithArg(ARMInterpreter* interpreter, uint64_t fn,
                              uint32_t arg);
int ARMInterpreterRun(ARMInterpreter* interpreter);
void ARMInterpreterCycle(ARMInterpreter* interpreter);
void ARMInterpreterDestruct(ARMInterpreter* interpreter);
void ARMInterpreterDumpRegisters(ARMInterpreter* interpreter);

bool ARMGuestAddressExecutable(Loader* loader, uint64_t addr);
uint64_t ARMGuestFunctionRuntime(Loader* loader, uint64_t addr);
void* ARMGuestAddressToHost(ARMInterpreter* interpreter, uint64_t addr,
                            size_t size);
uint64_t ARMLookupGuestFunction(Loader* loader, const char* name);
void ARMGuestCallVoidFunction(ARMInterpreter* cpu, uint64_t fn);
void ARMGuestRunProgramFini(Loader* loader, ARMInterpreter* cpu);
bool ARMGuestRunProgramShutdown(Loader* loader, ARMInterpreter* cpu);
bool ARMGuestRunInitArrays(Loader* loader, ARMInterpreter* cpu);
bool ARMGuestRunFiniArrays(Loader* loader, ARMInterpreter* cpu);

int ARMGuestRunProgram(ARMInterpreter* interpreter, Loader* loader,
                       uint64_t entry_address, int argc, char** argv,
                       bool trace_regs, bool trace_instructions);

#endif /* arm_interpreter_h */
