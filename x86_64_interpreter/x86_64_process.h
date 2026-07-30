//
//  x86_64_process.h
//  x86_64_interpreter
//

#ifndef x86_64_process_h
#define x86_64_process_h

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "guest_addr_wait.h"
#include "loader.h"
#include "vector.h"
#include "x86_64_interpreter.h"

struct X86_64Runtime;

typedef enum {
  kGuestThreadIdle,
  kGuestThreadRunning,
  kGuestThreadFinished,
  kGuestThreadJoined,
} X86_64GuestThreadState;

typedef struct X86_64GuestMemoryRange {
  uint64_t start;
  uint64_t end;
} X86_64GuestMemoryRange;

typedef struct X86_64GuestThread {
  struct X86_64ProcessRuntime* process;
  uint64_t tid;
  pthread_t host_thread;
  bool host_thread_valid;
  bool is_main;
  bool joinable;
  bool detached;
  bool join_in_progress;
  X86_64GuestThreadState state;
  int exit_code;
  X86_64Interpreter cpu;
  char* stack;
  void* tls_block;
  size_t tls_block_size;
  uint64_t user_fn;
  uint64_t user_arg;
  uint64_t tls_init_fn;
  uint64_t tls_fini_fn;
  bool tls_fini_done;
  int heap_lock_depth;
} X86_64GuestThread;

typedef struct X86_64ProcessRuntime {
  struct X86_64Runtime* runtime;
  Loader* loader;
  pthread_mutex_t mutex;
  pthread_mutex_t got_resolve_mutex;
  pthread_mutex_t heap_mutex;
  pthread_rwlock_t memory_lock;
  pthread_key_t current_thread_key;
  Vector threads;
  Vector memory_ranges;
  uint64_t next_tid;
  int active_joins;
  X86_64GuestThread* main_thread;
  GuestAddrWaitTable addr_wait_table;
  bool shutting_down;
  bool initialized;
  bool addr_wait_table_initialized;
  bool mutex_initialized;
  bool got_resolve_mutex_initialized;
  bool heap_mutex_initialized;
  bool memory_lock_initialized;
  bool current_thread_key_initialized;
} X86_64ProcessRuntime;

void X86_64ProcessRuntimeInit(X86_64ProcessRuntime* process,
                              struct X86_64Runtime* runtime, Loader* loader);
void X86_64ProcessRuntimeDestruct(X86_64ProcessRuntime* process);

X86_64GuestThread* X86_64ProcessGetCurrentThread(X86_64ProcessRuntime* process);
void X86_64ProcessSetCurrentThread(X86_64ProcessRuntime* process,
                                 X86_64GuestThread* thread);

X86_64GuestThread* X86_64ProcessCreateMainThread(
    X86_64ProcessRuntime* process, uint64_t entry_address, int argc,
    char** argv, bool trace_registers, bool trace_instructions);

X86_64GuestThread* X86_64ProcessFindThread(X86_64ProcessRuntime* process,
                                           uint64_t tid);

int64_t X86_64SyscallThreadCreate(X86_64GuestThread* caller, uint64_t fn,
                                  uint64_t arg, uint64_t tls_init_fn,
                                  uint64_t tls_fini_fn);
int64_t X86_64SyscallThreadJoin(X86_64GuestThread* caller, uint64_t tid,
                                uint64_t result_ptr);
int64_t X86_64SyscallThreadDetach(X86_64GuestThread* caller, uint64_t tid);
int64_t X86_64SyscallAddrWait(X86_64GuestThread* caller, uint64_t address,
                              uint64_t expected_ptr, size_t size,
                              int64_t timeout_us);
int64_t X86_64SyscallAddrWake(X86_64GuestThread* caller, uint64_t address,
                              bool wake_all);
int64_t X86_64SyscallThreadSleep(X86_64GuestThread* caller,
                                 uint64_t duration_ptr, uint64_t remaining_ptr);
int64_t X86_64SyscallHardwareConcurrency(void);
int64_t X86_64SyscallThreadSelf(X86_64GuestThread* caller);
int64_t X86_64SyscallGetTp(X86_64GuestThread* caller);
void X86_64SyscallThreadExit(X86_64GuestThread* caller, int64_t status);
int64_t X86_64SyscallHeapLock(X86_64GuestThread* caller);
int64_t X86_64SyscallHeapUnlock(X86_64GuestThread* caller);

bool X86_64GuestAddressExecutable(Loader* loader, uint64_t addr);

uint64_t X86_64LookupGuestFunction(Loader* loader, const char* name);
void X86_64GuestCallVoidFunction(X86_64Interpreter* cpu, uint64_t fn);
void X86_64GuestRunProgramInit(Loader* loader, X86_64Interpreter* cpu);
void X86_64GuestRunProgramFini(Loader* loader, X86_64Interpreter* cpu);
bool X86_64GuestRunProgramShutdown(Loader* loader, X86_64Interpreter* cpu);
bool X86_64GuestRunInitArrays(Loader* loader, X86_64Interpreter* cpu);
bool X86_64GuestRunFiniArrays(Loader* loader, X86_64Interpreter* cpu);
int X86_64NativeCallVoidFunction(Loader* loader, uint64_t fn);

void X86_64ProcessRegisterGuestMemory(X86_64ProcessRuntime* process,
                                      uint64_t start, size_t size);
void X86_64ProcessUnregisterGuestMemory(X86_64ProcessRuntime* process,
                                        uint64_t start, size_t size);
bool X86_64ProcessGuestMemoryOk(X86_64ProcessRuntime* process, uint64_t addr,
                                size_t size);

#endif /* x86_64_process_h */
