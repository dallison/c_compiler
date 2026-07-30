//
// Shared process state for AArch64 guest interpreters.
//

#ifndef aarch64_process_h
#define aarch64_process_h

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "guest_addr_wait.h"
#include "loader.h"
#include "vector.h"
#include "aarch64_interpreter.h"

typedef enum {
  kAARCH64GuestThreadIdle,
  kAARCH64GuestThreadRunning,
  kAARCH64GuestThreadFinished,
  kAARCH64GuestThreadJoined,
} AARCH64GuestThreadState;

typedef struct AARCH64GuestMemoryRange {
  uint64_t start;
  uint64_t end;
} AARCH64GuestMemoryRange;

typedef struct AARCH64GuestThread {
  struct AARCH64ProcessRuntime* process;
  uint64_t tid;
  pthread_t host_thread;
  bool host_thread_valid;
  bool is_main;
  bool joinable;
  bool detached;
  bool join_in_progress;
  AARCH64GuestThreadState state;
  int exit_code;
  AARCH64Interpreter cpu;
  char* stack;
  void* tls_block;
  size_t tls_block_size;
  uint64_t user_fn;
  uint64_t user_arg;
  uint64_t tls_init_fn;
  uint64_t tls_fini_fn;
  bool tls_fini_done;
  int heap_lock_depth;
} AARCH64GuestThread;

typedef struct AARCH64ProcessRuntime {
  struct AARCH64Runtime* runtime;
  Loader* loader;
  // Serializes host accesses to guest memory.  write_epoch implements a
  // conservative process-wide ARM global monitor: every guest write
  // invalidates every outstanding exclusive reservation.
  pthread_mutex_t memory_mutex;
  uint64_t write_epoch;
  pthread_mutex_t mutex;
  pthread_mutex_t got_resolve_mutex;
  pthread_mutex_t heap_mutex;
  pthread_rwlock_t memory_lock;
  pthread_key_t current_thread_key;
  Vector threads;
  Vector memory_ranges;
  uint64_t next_tid;
  int active_joins;
  AARCH64GuestThread* main_thread;
  GuestAddrWaitTable addr_wait_table;
  bool shutting_down;
  bool initialized;
  bool addr_wait_table_initialized;
  bool memory_mutex_initialized;
  bool mutex_initialized;
  bool got_resolve_mutex_initialized;
  bool heap_mutex_initialized;
  bool memory_lock_initialized;
  bool current_thread_key_initialized;
} AARCH64ProcessRuntime;

void AARCH64ProcessRuntimeInit(AARCH64ProcessRuntime* process,
                               struct AARCH64Runtime* runtime, Loader* loader);
void AARCH64ProcessRuntimeDestruct(AARCH64ProcessRuntime* process);

AARCH64GuestThread* AARCH64ProcessGetCurrentThread(
    AARCH64ProcessRuntime* process);
void AARCH64ProcessSetCurrentThread(AARCH64ProcessRuntime* process,
                                    AARCH64GuestThread* thread);

AARCH64GuestThread* AARCH64ProcessCreateMainThread(
    AARCH64ProcessRuntime* process, uint64_t entry_address, int argc,
    char** argv, bool trace_registers, bool trace_instructions);

AARCH64GuestThread* AARCH64ProcessFindThread(AARCH64ProcessRuntime* process,
                                             uint64_t tid);

int64_t AARCH64SyscallThreadCreate(AARCH64GuestThread* caller, uint64_t fn,
                                   uint64_t arg, uint64_t tls_init_fn,
                                   uint64_t tls_fini_fn);
int64_t AARCH64SyscallThreadJoin(AARCH64GuestThread* caller, uint64_t tid,
                                 uint64_t result_ptr);
int64_t AARCH64SyscallThreadDetach(AARCH64GuestThread* caller, uint64_t tid);
int64_t AARCH64SyscallAddrWait(AARCH64GuestThread* caller, uint64_t address,
                               uint64_t expected_ptr, size_t size,
                               int64_t timeout_us);
int64_t AARCH64SyscallAddrWake(AARCH64GuestThread* caller, uint64_t address,
                               bool wake_all);
int64_t AARCH64SyscallThreadSleep(AARCH64GuestThread* caller,
                                   uint64_t duration_ptr, uint64_t remaining_ptr);
int64_t AARCH64SyscallHardwareConcurrency(void);
int64_t AARCH64SyscallThreadSelf(AARCH64GuestThread* caller);
int64_t AARCH64SyscallGetTp(AARCH64GuestThread* caller);
void AARCH64SyscallThreadExit(AARCH64GuestThread* caller, int64_t status);
int64_t AARCH64SyscallHeapLock(AARCH64GuestThread* caller);
int64_t AARCH64SyscallHeapUnlock(AARCH64GuestThread* caller);

void AARCH64ProcessRegisterGuestMemory(AARCH64ProcessRuntime* process,
                                       uint64_t start, size_t size);
void AARCH64ProcessUnregisterGuestMemory(AARCH64ProcessRuntime* process,
                                         uint64_t start, size_t size);
bool AARCH64ProcessGuestMemoryOk(AARCH64ProcessRuntime* process, uint64_t addr,
                                 size_t size);

#endif /* aarch64_process_h */
