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

struct X86_64ProcessRuntime;
struct X86_64GuestThread;

#define X86_64_STACK_SIZE (8 * 1024 * 1024)

typedef struct X86_64Interpreter {
  Loader* loader;
  struct X86_64ProcessRuntime* process;
  struct X86_64GuestThread* guest_thread;
  uint64_t guest_tid;
  uint64_t iregs[X86_NUM_INT_REGS];
  // SSE/XMM register file.  Each register holds 128 bits (two 64-bit lanes);
  // only the low lane(s) are used by scalar floating-point code.
  uint64_t xmm[16][2];
  uint64_t rsp;
  uint64_t rbp;
  uint64_t rip;
  char* stack;
  bool owns_stack;
  bool trace_instructions;
  bool trace_registers;
  uint64_t old_iregs[X86_NUM_INT_REGS];
  uint64_t old_rsp;
  uint64_t old_rbp;
  bool cf;
  bool zf;
  bool sf;
  bool of;
  bool pf;
  jmp_buf debugger;
  bool running;
  bool rip_updated;
  int exit_code;
  uint64_t fs_base;
  size_t tls_block_size;
  uint8_t current_seg_prefix;
  SymbolScope symbol_cache;
} X86_64Interpreter;

void X86_64InterpreterInit(X86_64Interpreter* interpreter, Loader* loader,
                           uint64_t entry_address, int argc, char** argv,
                           bool trace_registers, bool trace_instructions);

void X86_64InterpreterInitForThread(
    X86_64Interpreter* interpreter, struct X86_64ProcessRuntime* process,
    struct X86_64GuestThread* guest_thread, Loader* loader,
    uint64_t entry_address, int argc, char** argv, char* stack,
    uint64_t fs_base, size_t tls_block_size, bool trace_registers,
    bool trace_instructions);

void X86_64InterpreterWriteReg(X86_64Interpreter* interpreter, int reg,
                               uint64_t value);

void X86_64InterpreterPrepareCall(X86_64Interpreter* interpreter, uint64_t fn,
                                  uint64_t arg);

void X86_64InterpreterPrepareMain(X86_64Interpreter* interpreter,
                                  uint64_t entry_address, int argc,
                                  char** argv, bool is_static_link);

int X86_64InterpreterRunUntilReturn(X86_64Interpreter* interpreter);

int X86_64InterpreterCall(X86_64Interpreter* interpreter, uint64_t fn,
                          uint64_t arg);

int X86_64InterpreterRun(X86_64Interpreter* interpreter);
void X86_64InterpreterDestruct(X86_64Interpreter* interpreter);
void X86_64InterpreterDumpRegisters(X86_64Interpreter* interpreter);

SymbolScope* X86_64InterpreterFindSymbol(X86_64Interpreter* interpreter,
                                         uint64_t address);

void X86_64InterpreterFail(X86_64Interpreter* interpreter, int status);

#endif /* x86_64_interpreter_h */
