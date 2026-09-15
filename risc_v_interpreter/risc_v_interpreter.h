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
#include <stdint.h>

#include "loader.h"
#include "risc_v_machine.h"
#include "../libc/include/davecc_guest_syscalls.h"

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
#define RISC_V_ECALL_FS_STATUS 31
#define RISC_V_ECALL_FS_OPEN_DIRECTORY 32
#define RISC_V_ECALL_FS_READ_DIRECTORY 33
#define RISC_V_ECALL_FS_CLOSE_DIRECTORY 34
#define RISC_V_ECALL_FS_CREATE_DIRECTORY 35
#define RISC_V_ECALL_FS_REMOVE 36
#define RISC_V_ECALL_FS_RENAME 37
#define RISC_V_ECALL_FS_CURRENT_PATH 38
#define RISC_V_ECALL_FS_SET_CURRENT_PATH 39
#define RISC_V_ECALL_FS_READ_SYMLINK 40
#define RISC_V_ECALL_FS_CREATE_SYMLINK 41
#define RISC_V_ECALL_FS_CREATE_HARD_LINK 42
#define RISC_V_ECALL_FS_SET_PERMISSIONS 43
#define RISC_V_ECALL_FS_RESIZE 44
#define RISC_V_ECALL_FS_SET_MODIFICATION_TIME 45
#define RISC_V_ECALL_FS_SPACE 46
#define RISC_V_ECALL_FS_COPY_FILE 47
#define RISC_V_ECALL_FS_CANONICAL 48
#define RISC_V_ECALL_TZDB_VERSION 49
#define RISC_V_ECALL_TZDB_GENERATION 50
#define RISC_V_ECALL_TZDB_RELOAD 51
#define RISC_V_ECALL_TZDB_CURRENT_ZONE 52
#define RISC_V_ECALL_TZDB_ZONE_COUNT 53
#define RISC_V_ECALL_TZDB_ZONE_NAME 54
#define RISC_V_ECALL_TZDB_LOCATE_ZONE 55
#define RISC_V_ECALL_TZDB_SYS_INFO 56
#define RISC_V_ECALL_TZDB_LOCAL_INFO 57
#define RISC_V_ECALL_TZDB_LEAP_COUNT 58
#define RISC_V_ECALL_TZDB_LEAP_INFO 59
#define RISC_V_ECALL_RANDOM_BYTES 60
#define RISC_V_ECALL_FS_DESCRIPTOR_STATUS 61
#define RISC_V_ECALL_ENVIRONMENT_VALUE 62
#define RISC_V_ECALL_NESTED_RETURN 255

#define RISC_V_VALIDATE_DAVE_SYSCALL(name)                              \
  typedef char risc_v_dave_syscall_##name[                            \
      RISC_V_ECALL_##name == DAVE_SYS_##name ? 1 : -1];
DAVE_GUEST_SYSCALL_LIST(RISC_V_VALIDATE_DAVE_SYSCALL)
#undef RISC_V_VALIDATE_DAVE_SYSCALL

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
  double fregs[RV_NUM_FLOAT_REGS];
  uint8_t vregs[RV_NUM_VECTOR_REGS][RV_VLEN_BYTES];
  int vl;
  int sew_bytes;
  int64_t old_iregs[RV_NUM_INT_REGS];
  double old_fregs[RV_NUM_FLOAT_REGS];

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
