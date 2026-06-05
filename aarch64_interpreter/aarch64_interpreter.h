//
//  aarch64_interpreter.h
//  aarch64_interpreter
//

#ifndef aarch64_interpreter_h
#define aarch64_interpreter_h

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#include "aarch64_machine.h"
#include "loader.h"

#define AARCH64_STACK_SIZE (8 * 1024 * 1024)

typedef struct AARCH64Interpreter {
  Loader* loader;
  uint64_t x[31];
  // Scalar floating-point / SIMD register file.  Only the low 64 bits are
  // modelled (enough for float/double scalars).
  uint64_t v[32];
  uint64_t sp;
  uint64_t pc;
  bool flag_n;
  bool flag_z;
  bool flag_c;
  bool flag_v;
  char* stack;
  bool trace_instructions;
  bool trace_registers;
  uint64_t old_x[31];
  uint64_t old_sp;
  jmp_buf debugger;
  bool running;
  int exit_code;
  uint32_t symbol_resolver_code[2];
} AARCH64Interpreter;

void AARCH64InterpreterInit(AARCH64Interpreter* interpreter, Loader* loader,
                            uint64_t entry_address, int argc, char** argv,
                            bool trace_registers, bool trace_instructions);
int AARCH64InterpreterRun(AARCH64Interpreter* interpreter);
void AARCH64InterpreterDestruct(AARCH64Interpreter* interpreter);
void AARCH64InterpreterDumpRegisters(AARCH64Interpreter* interpreter);

#endif /* aarch64_interpreter_h */
