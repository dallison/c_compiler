#ifndef risc_v_process_h
#define risc_v_process_h

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "loader.h"
#include "risc_v_interpreter.h"
#include "vector.h"

#define RISC_V_MAX_GUEST_THREADS 16
#define RISC_V_STACK_BASE 0x70000000ull
#define RISC_V_TLS_BASE 0x60000000ull

typedef enum {
  kRISCVGuestThreadIdle,
  kRISCVGuestThreadRunning,
  kRISCVGuestThreadFinished,
  kRISCVGuestThreadJoined,
} RISCVGuestThreadState;

typedef struct RISCVGuestThread {
  struct RISCVProcessRuntime* process;
  uint64_t tid;
  pthread_t host_thread;
  bool host_thread_valid;
  bool is_main;
  bool joinable;
  RISCVGuestThreadState state;
  int exit_code;
  RISCVInterpreter cpu_storage;
  RISCVInterpreter* cpu;
  char* stack;
  void* tls_block;
  size_t tls_block_size;
  uint64_t user_fn;
  uint64_t user_arg;
  uint64_t tls_init_fn;
  uint64_t tls_fini_fn;
  bool tls_fini_done;
  int heap_lock_depth;
} RISCVGuestThread;

typedef struct RISCVProcessRuntime {
  Loader* loader;
  pthread_mutex_t memory_mutex;
  uint64_t write_epoch;
  pthread_mutex_t mutex;
  pthread_mutex_t got_resolve_mutex;
  pthread_mutex_t heap_mutex;
  pthread_key_t current_thread_key;
  Vector threads;
  uint64_t next_tid;
  RISCVGuestThread* main_thread;
  bool shutting_down;
  bool initialized;
} RISCVProcessRuntime;

// Provided by risc_v_interpreter.c during concurrent process integration.
void RISCVInterpreterInitForThread(
    RISCVInterpreter* interpreter, struct RISCVProcessRuntime* process,
    struct RISCVGuestThread* guest_thread, Loader* loader,
    uint64_t entry_address, int argc, char** argv, char* stack,
    void* tls_block, size_t tls_block_size, bool trace_regs,
    bool trace_instructions);
int RISCVInterpreterCallWithArg(RISCVInterpreter* interpreter, uint64_t fn,
                                uint64_t arg);
void* RISCVGuestAddressToHost(RISCVInterpreter* interpreter, uint64_t addr,
                              size_t size);

bool RISCVProcessRuntimeInit(RISCVProcessRuntime* process, Loader* loader);
void RISCVProcessRuntimeDestruct(RISCVProcessRuntime* process);
RISCVGuestThread* RISCVProcessAttachMainThread(RISCVProcessRuntime* process,
                                               RISCVInterpreter* cpu);
RISCVGuestThread* RISCVProcessGetCurrentThread(RISCVProcessRuntime* process);
void* RISCVProcessResolveGuestAddress(RISCVProcessRuntime* process,
                                      uint64_t addr, size_t size);

int64_t RISCVSyscallThreadCreate(RISCVGuestThread* caller, uint64_t fn,
                                 uint64_t arg, uint64_t tls_init_fn,
                                 uint64_t tls_fini_fn);
int64_t RISCVSyscallThreadJoin(RISCVGuestThread* caller, uint64_t tid,
                               uint64_t result_ptr);
int64_t RISCVSyscallThreadSelf(RISCVGuestThread* caller);
int64_t RISCVSyscallGetTp(RISCVGuestThread* caller);
void RISCVSyscallThreadExit(RISCVGuestThread* caller, int64_t status);
int64_t RISCVSyscallHeapLock(RISCVGuestThread* caller);
int64_t RISCVSyscallHeapUnlock(RISCVGuestThread* caller);
int64_t RISCVSyscallTime(void);
int64_t RISCVSyscallClock(void);

#endif
