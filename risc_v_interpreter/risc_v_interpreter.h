//
//  risc_v_interpreter.h
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_interpreter_h
#define risc_v_interpreter_h

#include <setjmp.h>

#include "loader.h"
#include "risc_v_machine.h"

#define RISC_V_STACK_SIZE 8*1024*1024

// Environment call codes (passed in t6(x31)).  These are the equivalent of
// system calls in an OS.
#define RISC_V_ECALL_HALT 1
#define RISC_V_ECALL_OPEN 2
#define RISC_V_ECALL_CLOSE 3
#define RISC_V_ECALL_WRITE 4
#define RISC_V_ECALL_READ 5
#define RISC_V_ECALL_RESOLVE 6
#define RISC_V_ECALL_LSEEK 7
#define RISC_V_ECALL_MALLOC 8
#define RISC_V_ECALL_FREE 9
#define RISC_V_ECALL_REALLOC 10
#define RISC_V_ECALL_ABORT 11
#define RISC_V_ECALL_EXIT 12
#define RISC_V_ECALL_TIME 13
#define RISC_V_ECALL_CLOCK 14
#define RISC_V_ECALL_THREAD_CREATE 15
#define RISC_V_ECALL_THREAD_JOIN 16
#define RISC_V_ECALL_THREAD_SELF 17
#define RISC_V_ECALL_GET_TP 18
#define RISC_V_ECALL_THREAD_EXIT 19
#define RISC_V_ECALL_HEAP_LOCK 20
#define RISC_V_ECALL_HEAP_UNLOCK 21
#define RISC_V_ECALL_EXIT_CLEAN 22
#define RISC_V_ECALL_THREAD_YIELD 23
#define RISC_V_ECALL_MONOTONIC_TIME 24
#define RISC_V_ECALL_THREAD_DETACH 25
#define RISC_V_ECALL_ADDR_WAIT 26
#define RISC_V_ECALL_ADDR_WAKE 27
#define RISC_V_ECALL_THREAD_SLEEP 28
#define RISC_V_ECALL_HARDWARE_CONCURRENCY 29
#define RISC_V_ECALL_REALTIME_TIME 30
#define RISC_V_ECALL_NESTED_RETURN 255

// Registers
#define RISC_V_REG_x0 0
#define RISC_V_REG_ra 1
#define RISC_V_REG_sp 2
#define RISC_V_REG_gp 3
#define RISC_V_REG_tp 4
#define RISC_V_REG_t0 5
#define RISC_V_REG_t1 6
#define RISC_V_REG_t2 7
#define RISC_V_REG_fp 8
#define RISC_V_REG_s0 8
#define RISC_V_REG_s1 9
#define RISC_V_REG_a0 10
#define RISC_V_REG_a1 11
#define RISC_V_REG_a2 12
#define RISC_V_REG_a3 13
#define RISC_V_REG_a4 14
#define RISC_V_REG_a5 15
#define RISC_V_REG_a6 16
#define RISC_V_REG_a7 17
#define RISC_V_REG_s2 18
#define RISC_V_REG_s3 19
#define RISC_V_REG_s4 20
#define RISC_V_REG_s5 21
#define RISC_V_REG_s6 22
#define RISC_V_REG_s7 23
#define RISC_V_REG_s8 24
#define RISC_V_REG_s9 25
#define RISC_V_REG_s10 26
#define RISC_V_REG_s11 27
#define RISC_V_REG_t3 28
#define RISC_V_REG_t4 29
#define RISC_V_REG_t5 30
#define RISC_V_REG_t6 31

#define REG(n) RISC_V_REG_##n

struct RISCVProcessRuntime;
struct RISCVGuestThread;

typedef struct RISCVInterpreter {
  Loader* loader;
  int64_t iregs[RV_NUM_INT_REGS];
  double fregs[RV_NUM_FLOAT_REGS];;
  int64_t old_iregs[RV_NUM_INT_REGS];
  double old_fregs[RV_NUM_FLOAT_REGS];;

  int32_t startup_code[3];
  int32_t call_return_code[2];
  int32_t symbol_resolver_code[2];
  char* stack;
  bool owns_stack;
  uint64_t stack_guest_base;
  void* tls_block;
  size_t tls_block_size;
  uint64_t tls_guest_base;
  struct RISCVProcessRuntime* process;
  struct RISCVGuestThread* guest_thread;
  int64_t pc;
  SymbolScope* current_symbol;
  bool trace_regs;
  bool trace_instructions;
  int64_t num_steps;
  bool running;
  int exit_code;
  bool reservation_valid;
  uint64_t reservation_address;
  uint32_t reservation_size;
  uint64_t reservation_epoch;
  jmp_buf debugger;
} RISCVInterpreter;

void RISCVInterpreterInit(RISCVInterpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv,
                          bool trace_regs, bool trace_instructions);
void RISCVInterpreterInitForThread(
    RISCVInterpreter* interpreter, struct RISCVProcessRuntime* process,
    struct RISCVGuestThread* guest_thread, Loader* loader,
    uint64_t entry_address, int argc, char** argv, char* stack,
    void* tls_block, size_t tls_block_size, bool trace_regs,
    bool trace_instructions);
void RISCVInterpreterPrepareMain(RISCVInterpreter* interpreter,
                                 uint64_t entry_address, int argc,
                                 char** argv);
void RISCVInterpreterCycle(RISCVInterpreter* interpreter);
int RISCVInterpreterRun(RISCVInterpreter* interpreter);
void RISCVInterpreterCall(RISCVInterpreter* interpreter, uint64_t fn);
int RISCVInterpreterCallWithArg(RISCVInterpreter* interpreter, uint64_t fn,
                                uint64_t arg);
void* RISCVGuestAddressToHost(RISCVInterpreter* interpreter, uint64_t addr,
                              size_t size);
void RISCVInterpreterDestruct(RISCVInterpreter* interpreter);
void RISCVInterpreterDumpRegisters(RISCVInterpreter* interpreter);

bool RISCVGuestAddressExecutable(Loader* loader, uint64_t addr);
uint64_t RISCVLookupGuestFunction(Loader* loader, const char* name);
void RISCVGuestCallVoidFunction(RISCVInterpreter* interpreter, uint64_t fn);
bool RISCVGuestRunInitArrays(Loader* loader, RISCVInterpreter* interpreter);
bool RISCVGuestRunFiniArrays(Loader* loader, RISCVInterpreter* interpreter);
void RISCVGuestRunProgramFini(Loader* loader, RISCVInterpreter* interpreter);
bool RISCVGuestRunProgramShutdown(Loader* loader,
                                  RISCVInterpreter* interpreter);

#endif /* risc_v_interpreter_h */
