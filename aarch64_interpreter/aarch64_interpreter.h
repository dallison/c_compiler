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

struct AARCH64ProcessRuntime;
struct AARCH64GuestThread;

typedef struct AARCH64Interpreter {
  Loader* loader;
  struct AARCH64ProcessRuntime* process;
  struct AARCH64GuestThread* guest_thread;
  uint64_t guest_tid;
  uint64_t tp_base;
  size_t tls_block_size;
  uint64_t x[31];
  // SIMD&FP register file.  Each register is 128 bits; scalar float/double
  // operations use the low 32/64 bits and zero the rest on write.
  uint64_t v[32][2];
  uint64_t sp;
  uint64_t pc;
  bool flag_n;
  bool flag_z;
  bool flag_c;
  bool flag_v;
  char* stack;
  bool owns_stack;
  bool trace_instructions;
  bool trace_registers;
  uint64_t old_x[31];
  uint64_t old_sp;
  jmp_buf debugger;
  bool running;
  int exit_code;
  uint32_t symbol_resolver_code[2];
  bool reservation_valid;
  uint64_t reservation_address;
  uint32_t reservation_size;
  uint64_t reservation_epoch;
} AARCH64Interpreter;

void AARCH64InterpreterInit(AARCH64Interpreter* interpreter, Loader* loader,
                            struct AARCH64ProcessRuntime* process,
                            uint64_t entry_address, int argc, char** argv,
                            bool trace_registers, bool trace_instructions);

void AARCH64InterpreterInitForThread(
    AARCH64Interpreter* interpreter, struct AARCH64ProcessRuntime* process,
    struct AARCH64GuestThread* guest_thread, Loader* loader,
    uint64_t entry_address, int argc, char** argv, char* stack,
    uint64_t tp_base, size_t tls_block_size, bool trace_registers,
    bool trace_instructions);

void AARCH64InterpreterWriteX(AARCH64Interpreter* interpreter, int reg,
                              uint64_t value);

void AARCH64InterpreterPrepareMain(AARCH64Interpreter* interpreter,
                                   uint64_t entry_address, int argc,
                                   char** argv, bool is_static_link);
void AARCH64InterpreterPrepareCall(AARCH64Interpreter* interpreter, uint64_t fn,
                                   uint64_t arg);
int AARCH64InterpreterCall(AARCH64Interpreter* interpreter, uint64_t fn,
                           uint64_t arg);
int AARCH64InterpreterRun(AARCH64Interpreter* interpreter);
void AARCH64InterpreterDestruct(AARCH64Interpreter* interpreter);
void AARCH64InterpreterDumpRegisters(AARCH64Interpreter* interpreter);

void AARCH64InterpreterFail(AARCH64Interpreter* interpreter, int status);

#endif /* aarch64_interpreter_h */
