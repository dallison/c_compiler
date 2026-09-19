//
//  x86_64_process.c
//  x86_64_interpreter
//

#include "x86_64_process.h"

#include "elf.h"
#include "guest_addr_wait.h"
#include "loader_arch.h"
#include "loader_lifecycle.h"
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t g_fallback_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int g_fallback_heap_lock_depth = 0;

static void GuestThreadDestruct(X86_64GuestThread* thread);

static bool GuestLoaderAddressOk(Loader* loader, uint64_t addr, size_t size) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return false;
}

static bool GuestMemoryReadable(X86_64ProcessRuntime* process, uint64_t addr,
                                size_t size) {
  if (process == NULL) {
    return false;
  }
  if (X86_64ProcessGuestMemoryOk(process, addr, size)) {
    return true;
  }
  return GuestLoaderAddressOk(process->loader, addr, size);
}

static bool GuestMemoryContainsLocked(X86_64ProcessRuntime* process,
                                      uint64_t addr, size_t size) {
  for (size_t i = 0; i < process->memory_ranges.length; i++) {
    X86_64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    if (range != NULL && addr >= range->start && addr + size <= range->end) {
      return true;
    }
  }
  return false;
}

static bool X86_64GuestAddrRead(void* context, uint64_t guest_address,
                                void* value, size_t size) {
  X86_64ProcessRuntime* process = context;
  if (process == NULL || !process->initialized ||
      !process->memory_lock_initialized) {
    return false;
  }
  pthread_rwlock_rdlock(&process->memory_lock);
  bool ok = GuestMemoryContainsLocked(process, guest_address, size);
  if (!ok) {
    ok = GuestLoaderAddressOk(process->loader, guest_address, size);
  }
  if (ok) {
    memcpy(value, (void*)(uintptr_t)guest_address, size);
  }
  pthread_rwlock_unlock(&process->memory_lock);
  return ok;
}

static void ProcessRemoveThread(X86_64ProcessRuntime* process,
                                X86_64GuestThread* thread) {
  for (size_t i = 0; i < process->threads.length; i++) {
    if (process->threads.value.p[i] == thread) {
      VectorDeleteElement(&process->threads, i);
      return;
    }
  }
}

static void ReapFinishedDetachedThreads(X86_64ProcessRuntime* process) {
  for (;;) {
    X86_64GuestThread* thread = NULL;
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      X86_64GuestThread* candidate = process->threads.value.p[i];
      if (candidate != NULL && candidate->detached &&
          candidate->state == kGuestThreadFinished &&
          candidate->host_thread_valid && !candidate->is_main) {
        thread = candidate;
        ProcessRemoveThread(process, thread);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    if (thread == NULL) {
      return;
    }
    pthread_join(thread->host_thread, NULL);
    GuestThreadDestruct(thread);
  }
}

void X86_64ProcessRegisterGuestMemory(X86_64ProcessRuntime* process,
                                      uint64_t start, size_t size) {
  if (process == NULL || !process->initialized || size == 0) {
    return;
  }
  X86_64GuestMemoryRange* range = malloc(sizeof(*range));
  if (range == NULL) {
    return;
  }
  range->start = start;
  range->end = start + (uint64_t)size;
  pthread_rwlock_wrlock(&process->memory_lock);
  VectorAppend(&process->memory_ranges, range);
  pthread_rwlock_unlock(&process->memory_lock);
}

void X86_64ProcessUnregisterGuestMemory(X86_64ProcessRuntime* process,
                                      uint64_t start, size_t size) {
  if (process == NULL || !process->initialized || size == 0 ||
      !process->memory_lock_initialized) {
    return;
  }
  uint64_t end = start + (uint64_t)size;
  pthread_rwlock_wrlock(&process->memory_lock);
  for (size_t i = 0; i < process->memory_ranges.length;) {
    X86_64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    if (range != NULL && range->start == start && range->end == end) {
      free(range);
      VectorDeleteElement(&process->memory_ranges, i);
      continue;
    }
    i++;
  }
  pthread_rwlock_unlock(&process->memory_lock);
}

bool X86_64ProcessGuestMemoryOk(X86_64ProcessRuntime* process, uint64_t addr,
                                size_t size) {
  if (process == NULL || !process->initialized) {
    return false;
  }
  pthread_rwlock_rdlock(&process->memory_lock);
  for (size_t i = 0; i < process->memory_ranges.length; i++) {
    X86_64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    if (range != NULL && addr >= range->start && addr + size <= range->end) {
      pthread_rwlock_unlock(&process->memory_lock);
      return true;
    }
  }
  pthread_rwlock_unlock(&process->memory_lock);
  return false;
}

static void RegisterThreadMemory(X86_64ProcessRuntime* process,
                                 X86_64GuestThread* thread) {
  char* stack = thread->stack != NULL ? thread->stack : thread->cpu.stack;
  if (stack != NULL) {
    X86_64ProcessRegisterGuestMemory(
        process, (uint64_t)(uintptr_t)stack, X86_64_STACK_SIZE);
  }
  if (thread->tls_block != NULL && thread->tls_block_size > 0) {
    X86_64ProcessRegisterGuestMemory(
        process, (uint64_t)(uintptr_t)thread->tls_block,
        thread->tls_block_size);
  }
}

static void GuestThreadDestruct(X86_64GuestThread* thread) {
  if (thread == NULL) {
    return;
  }
  X86_64ProcessRuntime* process = thread->process;
  char* stack = thread->stack;
  void* tls_block = thread->tls_block;
  size_t tls_block_size = thread->tls_block_size;
  if (process != NULL && process->initialized) {
    if (stack != NULL) {
      X86_64ProcessUnregisterGuestMemory(process, (uint64_t)(uintptr_t)stack,
                                         X86_64_STACK_SIZE);
    }
    if (tls_block != NULL && tls_block_size > 0) {
      X86_64ProcessUnregisterGuestMemory(process, (uint64_t)(uintptr_t)tls_block,
                                         tls_block_size);
    }
  }
  X86_64InterpreterDestruct(&thread->cpu);
  free(stack);
  free(tls_block);
  free(thread);
}

void X86_64InterpreterFail(X86_64Interpreter* interpreter, int status) {
  if (interpreter != NULL && interpreter->guest_thread != NULL &&
      !interpreter->guest_thread->is_main) {
    interpreter->running = false;
    interpreter->rip = 0;
    X86_64SyscallThreadExit(interpreter->guest_thread, status);
    return;
  }
  exit(status);
}

static void DestroyProcessLocks(X86_64ProcessRuntime* process) {
  if (process->current_thread_key_initialized) {
    pthread_key_delete(process->current_thread_key);
    process->current_thread_key_initialized = false;
  }
  if (process->memory_lock_initialized) {
    pthread_rwlock_destroy(&process->memory_lock);
    process->memory_lock_initialized = false;
  }
  if (process->heap_mutex_initialized) {
    pthread_mutex_destroy(&process->heap_mutex);
    process->heap_mutex_initialized = false;
  }
  if (process->got_resolve_mutex_initialized) {
    pthread_mutex_destroy(&process->got_resolve_mutex);
    process->got_resolve_mutex_initialized = false;
  }
  if (process->mutex_initialized) {
    pthread_mutex_destroy(&process->mutex);
    process->mutex_initialized = false;
  }
}

static int RunWorkerGuestCalls(X86_64GuestThread* thread) {
  if (thread->tls_init_fn != 0) {
    X86_64InterpreterCall(&thread->cpu, thread->tls_init_fn, 0);
    if (thread->tls_fini_done) {
      return thread->cpu.exit_code;
    }
  }
  X86_64InterpreterCall(&thread->cpu, thread->user_fn, thread->user_arg);
  int result = thread->cpu.exit_code;
  if (!thread->tls_fini_done && thread->tls_fini_fn != 0) {
    thread->tls_fini_done = true;
    X86_64InterpreterCall(&thread->cpu, thread->tls_fini_fn, 0);
  }
  return result;
}

static void WaitForActiveJoins(X86_64ProcessRuntime* process) {
  for (;;) {
    pthread_mutex_lock(&process->mutex);
    int pending = process->active_joins;
    pthread_mutex_unlock(&process->mutex);
    if (pending == 0) {
      return;
    }
    sched_yield();
  }
}

static void WaitForRunningWorkers(X86_64ProcessRuntime* process) {
  for (;;) {
    ReapFinishedDetachedThreads(process);
    bool pending = false;
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      X86_64GuestThread* thread = process->threads.value.p[i];
      if (thread != NULL && !thread->is_main && thread->host_thread_valid &&
          thread->state != kGuestThreadFinished) {
        pending = true;
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    if (!pending) {
      return;
    }
    sched_yield();
  }
}

static void* GuestThreadHostEntry(void* arg) {
  X86_64GuestThread* thread = (X86_64GuestThread*)arg;
  X86_64ProcessSetCurrentThread(thread->process, thread);

  pthread_mutex_lock(&thread->process->mutex);
  thread->state = kGuestThreadRunning;
  pthread_mutex_unlock(&thread->process->mutex);

  int exit_code = RunWorkerGuestCalls(thread);

  if (thread->heap_lock_depth > 0 && thread->process != NULL &&
      thread->process->heap_mutex_initialized) {
    thread->heap_lock_depth = 0;
    pthread_mutex_unlock(&thread->process->heap_mutex);
  }

  pthread_mutex_lock(&thread->process->mutex);
  thread->exit_code = exit_code;
  thread->state = kGuestThreadFinished;
  pthread_mutex_unlock(&thread->process->mutex);

  return NULL;
}

static X86_64GuestThread* NewGuestThread(X86_64ProcessRuntime* process,
                                         uint64_t tid, bool is_main) {
  X86_64GuestThread* thread = calloc(1, sizeof(*thread));
  if (thread == NULL) {
    return NULL;
  }
  thread->process = process;
  thread->tid = tid;
  thread->is_main = is_main;
  thread->joinable = !is_main;
  thread->state = kGuestThreadIdle;
  thread->exit_code = 0;
  return thread;
}

void X86_64ProcessRuntimeInit(X86_64ProcessRuntime* process,
                              struct X86_64Runtime* runtime, Loader* loader) {
  memset(process, 0, sizeof(*process));
  process->runtime = runtime;
  process->loader = loader;
  if (pthread_mutex_init(&process->mutex, NULL) != 0) {
    return;
  }
  process->mutex_initialized = true;
  if (pthread_mutex_init(&process->got_resolve_mutex, NULL) != 0) {
    DestroyProcessLocks(process);
    return;
  }
  process->got_resolve_mutex_initialized = true;
  if (pthread_mutex_init(&process->heap_mutex, NULL) != 0) {
    DestroyProcessLocks(process);
    return;
  }
  process->heap_mutex_initialized = true;
  if (pthread_rwlock_init(&process->memory_lock, NULL) != 0) {
    DestroyProcessLocks(process);
    return;
  }
  process->memory_lock_initialized = true;
  if (pthread_key_create(&process->current_thread_key, NULL) != 0) {
    DestroyProcessLocks(process);
    return;
  }
  process->current_thread_key_initialized = true;
  if (!GuestAddrWaitTableInit(&process->addr_wait_table)) {
    DestroyProcessLocks(process);
    return;
  }
  process->addr_wait_table_initialized = true;
  VectorInit(&process->threads);
  VectorInit(&process->memory_ranges);
  process->next_tid = 1;
  process->initialized = true;

  if (loader->tls.present && loader->tls.block_size > 0) {
    X86_64ProcessRegisterGuestMemory(process, loader->tls.fs_base,
                                     loader->tls.block_size);
  }
}

void X86_64ProcessRuntimeDestruct(X86_64ProcessRuntime* process) {
  if (!process->initialized) {
    DestroyProcessLocks(process);
    return;
  }

  pthread_mutex_lock(&process->mutex);
  process->shutting_down = true;
  pthread_mutex_unlock(&process->mutex);

  if (process->addr_wait_table_initialized) {
    GuestAddrWaitTableShutdown(&process->addr_wait_table);
  }

  ReapFinishedDetachedThreads(process);
  WaitForRunningWorkers(process);
  WaitForActiveJoins(process);

  X86_64GuestThread* main_thread = NULL;
  pthread_mutex_lock(&process->mutex);
  for (size_t i = 0; i < process->threads.length; i++) {
    X86_64GuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->is_main) {
      main_thread = thread;
      break;
    }
  }
  pthread_mutex_unlock(&process->mutex);

  for (;;) {
    X86_64GuestThread* thread = NULL;
    pthread_t host = (pthread_t)0;
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      X86_64GuestThread* candidate = process->threads.value.p[i];
      if (candidate != NULL && !candidate->is_main &&
          candidate->host_thread_valid && !candidate->join_in_progress) {
        thread = candidate;
        host = candidate->host_thread;
        ProcessRemoveThread(process, thread);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    if (thread == NULL) {
      break;
    }
    pthread_join(host, NULL);
    GuestThreadDestruct(thread);
  }

  pthread_mutex_lock(&process->mutex);
  VectorClear(&process->threads);
  if (main_thread != NULL) {
    VectorAppend(&process->threads, main_thread);
  }
  pthread_mutex_unlock(&process->mutex);

  if (main_thread != NULL) {
    GuestThreadDestruct(main_thread);
  }
  VectorDestruct(&process->threads);

  for (size_t i = 0; i < process->memory_ranges.length; i++) {
    X86_64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    free(range);
  }
  VectorDestruct(&process->memory_ranges);

  if (process->addr_wait_table_initialized) {
    GuestAddrWaitTableDestruct(&process->addr_wait_table);
    process->addr_wait_table_initialized = false;
  }

  DestroyProcessLocks(process);
  process->initialized = false;
}

X86_64GuestThread* X86_64ProcessGetCurrentThread(
    X86_64ProcessRuntime* process) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  return (X86_64GuestThread*)pthread_getspecific(process->current_thread_key);
}

void X86_64ProcessSetCurrentThread(X86_64ProcessRuntime* process,
                                 X86_64GuestThread* thread) {
  pthread_setspecific(process->current_thread_key, thread);
}

X86_64GuestThread* X86_64ProcessFindThread(X86_64ProcessRuntime* process,
                                           uint64_t tid) {
  if (process == NULL || tid == 0) {
    return NULL;
  }
  for (size_t i = 0; i < process->threads.length; i++) {
    X86_64GuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->tid == tid) {
      return thread;
    }
  }
  return NULL;
}

bool X86_64GuestAddressExecutable(Loader* loader, uint64_t addr) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    if (region->segment->type != PT(load) ||
        (region->segment->flags & PF(x)) == 0) {
      continue;
    }
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr < end) {
      return true;
    }
  }
  return false;
}

static bool GuestExecutableOrZero(Loader* loader, uint64_t addr) {
  return addr == 0 || X86_64GuestAddressExecutable(loader, addr);
}

uint64_t X86_64LookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !X86_64GuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

void X86_64GuestCallVoidFunction(X86_64Interpreter* cpu, uint64_t fn) {
  if (cpu == NULL || fn == 0) {
    return;
  }
  X86_64InterpreterCall(cpu, fn, 0);
}

void X86_64GuestRunProgramInit(Loader* loader, X86_64Interpreter* cpu) {
  X86_64GuestCallVoidFunction(cpu,
                              X86_64LookupGuestFunction(loader,
                                                          "__davecc_program_init"));
}

void X86_64GuestRunProgramFini(Loader* loader, X86_64Interpreter* cpu) {
  X86_64GuestCallVoidFunction(cpu,
                              X86_64LookupGuestFunction(loader,
                                                          "__davecc_run_fini"));
}

bool X86_64GuestRunProgramShutdown(Loader* loader, X86_64Interpreter* cpu) {
  if (!LoaderLifecycleExecutableFiniAlreadyDone(loader->lifecycle)) {
    uint64_t guest_fini = X86_64LookupGuestFunction(loader, "__davecc_run_fini");
    if (guest_fini != 0) {
      if (cpu != NULL) {
        X86_64GuestCallVoidFunction(cpu, guest_fini);
      } else {
        X86_64NativeCallVoidFunction(loader, guest_fini);
      }
      LoaderLifecycleMarkExecutableFiniComplete(loader, loader->lifecycle);
    }
  }
  return X86_64GuestRunFiniArrays(loader, cpu);
}

static bool X86_64LifecycleCallback(void* context, LoadedDynamicLibrary* image,
                                      uint64_t function,
                                      LoaderLifecyclePhase phase) {
  (void)phase;
  typedef struct {
    Loader* loader;
    X86_64Interpreter* cpu;
  } X86_64LifecycleContext;
  X86_64LifecycleContext* ctx = context;
  uint64_t call_addr = function;
  if (ctx->loader->arch->ignore_vaddr) {
    if (!LoaderLinkedAddressToRuntime(ctx->loader, image, function,
                                      &call_addr)) {
      LoaderError("Cannot translate function array entry 0x%llx\n",
                  (unsigned long long)function);
      return false;
    }
  }
  if (!X86_64GuestAddressExecutable(ctx->loader, call_addr)) {
    LoaderError("Function array entry 0x%llx is not executable\n",
                (unsigned long long)call_addr);
    return false;
  }
  if (ctx->cpu != NULL) {
    X86_64GuestCallVoidFunction(ctx->cpu, call_addr);
  } else {
    X86_64NativeCallVoidFunction(ctx->loader, call_addr);
  }
  return true;
}

static bool RunGuestLifecyclePhase(Loader* loader, X86_64Interpreter* cpu,
                                   LoaderLifecyclePhase phase) {
  typedef struct {
    Loader* loader;
    X86_64Interpreter* cpu;
  } X86_64LifecycleContext;
  X86_64LifecycleContext ctx = {loader, cpu};
  return LoaderLifecycleRunPhase(loader, loader->lifecycle, phase,
                                 X86_64LifecycleCallback, &ctx);
}

bool X86_64GuestRunInitArrays(Loader* loader, X86_64Interpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecyclePreinit) &&
         RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleInit);
}

bool X86_64GuestRunFiniArrays(Loader* loader, X86_64Interpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleFini);
}

int X86_64NativeCallVoidFunction(Loader* loader, uint64_t fn) {
  if (fn == 0 || !GuestExecutableOrZero(loader, fn)) {
    return 0;
  }
  typedef void (*GuestVoidFn)(void);
  GuestVoidFn guest_fn = (GuestVoidFn)(uintptr_t)fn;
  guest_fn();
  return 0;
}

X86_64GuestThread* X86_64ProcessCreateMainThread(
    X86_64ProcessRuntime* process, uint64_t entry_address, int argc,
    char** argv, bool trace_registers, bool trace_instructions) {
  X86_64GuestThread* thread =
      NewGuestThread(process, process->next_tid++, true);
  if (thread == NULL) {
    return NULL;
  }
  thread->host_thread = pthread_self();
  thread->host_thread_valid = true;
  thread->state = kGuestThreadRunning;
  VectorAppend(&process->threads, thread);
  process->main_thread = thread;

  thread->tls_fini_fn =
      X86_64LookupGuestFunction(process->loader, "__davecc_tls_thread_fini");

  X86_64InterpreterInitForThread(&thread->cpu, process, thread, process->loader,
                                 entry_address, argc, argv, NULL, 0, 0,
                                 trace_registers, trace_instructions);
  thread->cpu.guest_tid = thread->tid;
  RegisterThreadMemory(process, thread);
  X86_64ProcessSetCurrentThread(process, thread);
  return thread;
}

static bool InitWorkerInterpreter(X86_64GuestThread* thread) {
  thread->stack = malloc(X86_64_STACK_SIZE);
  if (thread->stack == NULL) {
    return false;
  }

  uint64_t fs_base = 0;
  size_t tls_block_size = 0;
  if (thread->process->loader->tls.present) {
    if (!LoaderAllocThreadTlsBlock(thread->process->loader, &thread->tls_block,
                                   &thread->tls_block_size, &fs_base)) {
      return false;
    }
    tls_block_size = thread->tls_block_size;
  }

  X86_64InterpreterInitForThread(
      &thread->cpu, thread->process, thread, thread->process->loader, 0, 0, NULL,
      thread->stack, fs_base, tls_block_size, false, false);
  thread->cpu.running = false;
  thread->cpu.rip = 0;
  return true;
}

int64_t X86_64SyscallThreadCreate(X86_64GuestThread* caller, uint64_t fn,
                                  uint64_t arg, uint64_t tls_init_fn,
                                  uint64_t tls_fini_fn) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  X86_64ProcessRuntime* process = caller->process;
  Loader* loader = process->loader;

  if (!X86_64GuestAddressExecutable(loader, fn) ||
      !GuestExecutableOrZero(loader, tls_init_fn) ||
      !GuestExecutableOrZero(loader, tls_fini_fn)) {
    return -EINVAL;
  }

  X86_64GuestThread* thread = NewGuestThread(process, 0, false);
  if (thread == NULL) {
    return -ENOMEM;
  }
  thread->user_fn = fn;
  thread->user_arg = arg;
  thread->tls_init_fn = tls_init_fn;
  thread->tls_fini_fn = tls_fini_fn;
  thread->joinable = true;

  if (!InitWorkerInterpreter(thread)) {
    GuestThreadDestruct(thread);
    return -ENOMEM;
  }

  pthread_mutex_lock(&process->mutex);
  if (process->shutting_down) {
    pthread_mutex_unlock(&process->mutex);
    GuestThreadDestruct(thread);
    return -EINVAL;
  }
  thread->tid = process->next_tid++;
  thread->cpu.guest_tid = thread->tid;
  VectorAppend(&process->threads, thread);
  RegisterThreadMemory(process, thread);

  int rc = pthread_create(&thread->host_thread, NULL, GuestThreadHostEntry,
                          thread);
  if (rc != 0) {
    VectorPop(&process->threads);
    pthread_mutex_unlock(&process->mutex);
    GuestThreadDestruct(thread);
    return -ENOMEM;
  }
  thread->host_thread_valid = true;
  uint64_t tid = thread->tid;
  pthread_mutex_unlock(&process->mutex);
  ReapFinishedDetachedThreads(process);
  return (int64_t)tid;
}

static bool GuestJoinResultPointerOk(X86_64GuestThread* caller,
                                     uint64_t result_ptr) {
  if (result_ptr == 0) {
    return true;
  }
  X86_64ProcessRuntime* process = caller->process;
  if (X86_64ProcessGuestMemoryOk(process, result_ptr, sizeof(int))) {
    return true;
  }
  return GuestLoaderAddressOk(process->loader, result_ptr, sizeof(int));
}

int64_t X86_64SyscallThreadJoin(X86_64GuestThread* caller, uint64_t tid,
                                uint64_t result_ptr) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  if (tid == 0 || tid == caller->tid) {
    return -EINVAL;
  }
  if (!GuestJoinResultPointerOk(caller, result_ptr)) {
    return -EINVAL;
  }

  X86_64ProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  X86_64GuestThread* thread = X86_64ProcessFindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (!thread->joinable || thread->detached ||
      thread->state == kGuestThreadJoined) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  if (thread->is_main) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  if (!thread->host_thread_valid) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  thread->joinable = false;
  thread->join_in_progress = true;
  process->active_joins++;
  pthread_t host_thread = thread->host_thread;
  pthread_mutex_unlock(&process->mutex);

  pthread_join(host_thread, NULL);

  int exit_code = thread->exit_code;
  if (result_ptr != 0) {
    *(int*)(uintptr_t)result_ptr = exit_code;
  }

  pthread_mutex_lock(&process->mutex);
  thread->host_thread_valid = false;
  thread->join_in_progress = false;
  process->active_joins--;
  thread->state = kGuestThreadJoined;
  ProcessRemoveThread(process, thread);
  pthread_mutex_unlock(&process->mutex);

  GuestThreadDestruct(thread);
  ReapFinishedDetachedThreads(process);
  return 0;
}

int64_t X86_64SyscallThreadDetach(X86_64GuestThread* caller, uint64_t tid) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  if (tid == 0) {
    return -EINVAL;
  }

  X86_64ProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  X86_64GuestThread* thread = X86_64ProcessFindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (thread->is_main || thread->detached ||
      thread->state == kGuestThreadJoined) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  if (!thread->joinable) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  thread->joinable = false;
  thread->detached = true;
  pthread_mutex_unlock(&process->mutex);

  ReapFinishedDetachedThreads(process);
  return 0;
}

int64_t X86_64SyscallAddrWait(X86_64GuestThread* caller, uint64_t address,
                              uint64_t expected_ptr, size_t size,
                              int64_t timeout_us) {
  if (caller == NULL || caller->process == NULL ||
      !caller->process->addr_wait_table_initialized) {
    return -EINVAL;
  }
  if (expected_ptr == 0 || timeout_us < -1 ||
      (size != 1 && size != 2 && size != 4 && size != 8)) {
    return -EINVAL;
  }
  X86_64ProcessRuntime* process = caller->process;
  if (!GuestMemoryReadable(process, expected_ptr, size)) {
    return -EINVAL;
  }
  unsigned char expected[8];
  memcpy(expected, (void*)(uintptr_t)expected_ptr, size);
  return GuestAddrWait(&process->addr_wait_table, X86_64GuestAddrRead, process,
                       address, expected, size, timeout_us);
}

int64_t X86_64SyscallAddrWake(X86_64GuestThread* caller, uint64_t address,
                              bool wake_all) {
  if (caller == NULL || caller->process == NULL ||
      !caller->process->addr_wait_table_initialized) {
    return -EINVAL;
  }
  return GuestAddrWake(&caller->process->addr_wait_table, address, wake_all);
}

int64_t X86_64SyscallThreadSleep(X86_64GuestThread* caller,
                                 uint64_t duration_ptr, uint64_t remaining_ptr) {
  if (caller == NULL || caller->process == NULL || duration_ptr == 0) {
    return -EINVAL;
  }
  X86_64ProcessRuntime* process = caller->process;
  if (!GuestMemoryReadable(process, duration_ptr, sizeof(struct timespec))) {
    return -EINVAL;
  }
  struct timespec duration;
  memcpy(&duration, (void*)(uintptr_t)duration_ptr, sizeof(duration));
  if (duration.tv_sec < 0 || duration.tv_nsec < 0 ||
      duration.tv_nsec >= 1000000000L) {
    return -EINVAL;
  }
  struct timespec remaining_storage;
  struct timespec* remaining = NULL;
  if (remaining_ptr != 0) {
    if (!GuestMemoryReadable(process, remaining_ptr, sizeof(struct timespec))) {
      return -EINVAL;
    }
    remaining = &remaining_storage;
  }
  if (nanosleep(&duration, remaining) != 0) {
    if (remaining != NULL && remaining_ptr != 0) {
      memcpy((void*)(uintptr_t)remaining_ptr, remaining, sizeof(*remaining));
    }
    return -errno;
  }
  return 0;
}

int64_t X86_64SyscallHardwareConcurrency(void) {
  long count = sysconf(_SC_NPROCESSORS_ONLN);
  return count > 0 ? count : 0;
}

int64_t X86_64SyscallThreadSelf(X86_64GuestThread* caller) {
  if (caller == NULL) {
    return 0;
  }
  return (int64_t)caller->tid;
}

int64_t X86_64SyscallGetTp(X86_64GuestThread* caller) {
  if (caller == NULL || caller->cpu.fs_base == 0) {
    return 0;
  }
  return (int64_t)caller->cpu.fs_base;
}

void X86_64SyscallThreadExit(X86_64GuestThread* caller, int64_t status) {
  if (caller == NULL) {
    return;
  }
  if (!caller->tls_fini_done && caller->tls_fini_fn != 0) {
    caller->tls_fini_done = true;
    X86_64InterpreterCall(&caller->cpu, caller->tls_fini_fn, 0);
  }
  if (caller->heap_lock_depth > 0 && caller->process != NULL &&
      caller->process->heap_mutex_initialized) {
    caller->heap_lock_depth = 0;
    pthread_mutex_unlock(&caller->process->heap_mutex);
  }
  if (caller->is_main) {
    exit((int)status);
  }
  caller->exit_code = (int)status;
  caller->cpu.exit_code = (int)status;
  X86_64InterpreterWriteReg(&caller->cpu, X86_REG_RAX, (uint64_t)(int)status);
  caller->cpu.running = false;
  caller->cpu.rip = 0;
}

int64_t X86_64SyscallHeapLock(X86_64GuestThread* caller) {
  if (caller != NULL && caller->process != NULL &&
      caller->process->heap_mutex_initialized) {
    if (caller->heap_lock_depth == 0) {
      pthread_mutex_lock(&caller->process->heap_mutex);
    }
    caller->heap_lock_depth++;
    return 0;
  }
  if (g_fallback_heap_lock_depth == 0) {
    pthread_mutex_lock(&g_fallback_heap_mutex);
  }
  g_fallback_heap_lock_depth++;
  return 0;
}

int64_t X86_64SyscallHeapUnlock(X86_64GuestThread* caller) {
  if (caller != NULL && caller->process != NULL &&
      caller->process->heap_mutex_initialized) {
    if (caller->heap_lock_depth <= 0) {
      return -EINVAL;
    }
    caller->heap_lock_depth--;
    if (caller->heap_lock_depth == 0) {
      pthread_mutex_unlock(&caller->process->heap_mutex);
    }
    return 0;
  }
  if (g_fallback_heap_lock_depth <= 0) {
    return -EINVAL;
  }
  g_fallback_heap_lock_depth--;
  if (g_fallback_heap_lock_depth == 0) {
    pthread_mutex_unlock(&g_fallback_heap_mutex);
  }
  return 0;
}
