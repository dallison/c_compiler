//
//  x86_64_interpreter.h
//  x86_64_interpreter
//

#ifndef x86_64_interpreter_h
#define x86_64_interpreter_h

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#include "loader.h"
#include "x86_64_machine.h"

#define X86_64_STACK_SIZE (8 * 1024 * 1024)

typedef struct X86_64Interpreter {
  Loader* loader;
  uint64_t iregs[X86_NUM_INT_REGS];
  uint64_t rsp;
  uint64_t rbp;
  uint64_t rip;
  char* stack;
  bool trace_instructions;
  bool trace_registers;
  uint64_t old_iregs[X86_NUM_INT_REGS];
  uint64_t old_rsp;
  uint64_t old_rbp;
  bool cf;
  bool zf;
  bool sf;
  bool of;
  jmp_buf debugger;
  bool running;
  bool rip_updated;
  int exit_code;
} X86_64Interpreter;

void X86_64InterpreterInit(X86_64Interpreter* interpreter, Loader* loader,
                           uint64_t entry_address, int argc, char** argv,
                           bool trace_registers, bool trace_instructions);
int X86_64InterpreterRun(X86_64Interpreter* interpreter);
void X86_64InterpreterDestruct(X86_64Interpreter* interpreter);
void X86_64InterpreterDumpRegisters(X86_64Interpreter* interpreter);

#endif /* x86_64_interpreter_h */
