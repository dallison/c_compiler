#ifndef arm_process_h
#define arm_process_h

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "arm_interpreter.h"
#include "guest_addr_wait.h"
#include "loader.h"
#include "vector.h"

typedef enum {
  kARMGuestThreadIdle,
  kARMGuestThreadRunning,
  kARMGuestThreadFinished,
} ARMGuestThreadState;

typedef struct ARMGuestThread {
  struct ARMProcessRuntime* process;
  uint32_t tid;
  uint32_t slot;
  pthread_t host_thread;
  bool host_thread_valid;
  bool is_main;
  bool joinable;
  bool join_in_progress;
  ARMGuestThreadState state;
  int exit_code;
  ARMInterpreter cpu_storage;
  ARMInterpreter* cpu;
  char* stack;
  void* tls_block;
  size_t tls_block_size;
  uint64_t user_fn;
  uint32_t user_arg;
  uint64_t tls_init_fn;
  uint64_t tls_fini_fn;
  bool tls_fini_done;
  int heap_lock_depth;
  bool detached;
} ARMGuestThread;

typedef struct ARMProcessRuntime {
  Loader* loader;
  pthread_mutex_t memory_mutex;
  uint64_t write_epoch;
  pthread_mutex_t mutex;
  pthread_mutex_t got_resolve_mutex;
  pthread_mutex_t heap_mutex;
  pthread_key_t current_thread_key;
  Vector threads;
  uint32_t next_tid;
  int active_joins;
  ARMGuestThread* main_thread;
  GuestAddrWaitTable addr_wait_table;
  bool shutting_down;
  bool initialized;
} ARMProcessRuntime;

// ARMProcessResolveGuestAddress acquires process->mutex internally.
bool ARMProcessRuntimeInit(ARMProcessRuntime* process, Loader* loader);
void ARMProcessRuntimeDestruct(ARMProcessRuntime* process);
ARMGuestThread* ARMProcessAttachMainThread(ARMProcessRuntime* process,
                                           ARMInterpreter* cpu);
ARMGuestThread* ARMProcessGetCurrentThread(ARMProcessRuntime* process);
void* ARMProcessResolveGuestAddress(ARMProcessRuntime* process, uint64_t addr,
                                    size_t size);

int32_t ARMSyscallThreadCreate(ARMGuestThread* caller, uint32_t fn,
                               uint32_t arg, uint32_t tls_init_fn,
                               uint32_t tls_fini_fn);
int32_t ARMSyscallThreadJoin(ARMGuestThread* caller, uint32_t tid,
                             uint32_t result_ptr);
int32_t ARMSyscallThreadSelf(ARMGuestThread* caller);
int32_t ARMSyscallGetTp(ARMGuestThread* caller);
void ARMSyscallThreadExit(ARMGuestThread* caller, int32_t status);
int32_t ARMSyscallHeapLock(ARMGuestThread* caller);
int32_t ARMSyscallHeapUnlock(ARMGuestThread* caller);
int32_t ARMSyscallThreadDetach(ARMGuestThread* caller, uint32_t tid);
int32_t ARMSyscallAddrWait(ARMGuestThread* caller, uint32_t address,
                           uint32_t expected_ptr, uint32_t size,
                           int64_t timeout_us);
int32_t ARMSyscallAddrWake(ARMGuestThread* caller, uint32_t address,
                           uint32_t wake_all);
int32_t ARMSyscallThreadSleep(ARMGuestThread* caller, uint32_t duration_ptr,
                              uint32_t remaining_ptr);
int32_t ARMSyscallHardwareConcurrency(void);

#endif
