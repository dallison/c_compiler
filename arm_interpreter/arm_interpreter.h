//
//  arm_interpreter.h
//  arm_interpreter
//

#ifndef arm_interpreter_h
#define arm_interpreter_h

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#include "arm_machine.h"
#include "loader.h"

#define ARM_STACK_SIZE (8 * 1024 * 1024)
// Guest virtual address at which the interpreter maps the stack.  Chosen to sit
// above the code/data/heap (which start at 0x40000000) and below 0x80000000.
#define ARM_STACK_BASE 0x70000000u
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

typedef struct ARMInterpreter {
  Loader* loader;
  uint64_t regs[ARM_NUM_INT_REGS];
  uint32_t cpsr;
  float sregs[ARM_NUM_FLOAT_REGS];
  uint64_t old_regs[ARM_NUM_INT_REGS];
  float old_sregs[ARM_NUM_FLOAT_REGS];
  uint32_t symbol_resolver_code[2];
  char* stack;
  uint64_t pc;
  SymbolScope* current_symbol;
  bool trace_regs;
  bool trace_instructions;
  int num_steps;
  jmp_buf debugger;
} ARMInterpreter;

void ARMInterpreterInit(ARMInterpreter* interpreter, Loader* loader,
                        uint64_t entry_address, int argc, char** argv,
                        bool trace_regs, bool trace_instructions);
void ARMInterpreterCycle(ARMInterpreter* interpreter);
void ARMInterpreterDestruct(ARMInterpreter* interpreter);
void ARMInterpreterDumpRegisters(ARMInterpreter* interpreter);

#endif /* arm_interpreter_h */
