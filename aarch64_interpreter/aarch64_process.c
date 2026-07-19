//
// Shared process state for AArch64 guest interpreters.
//

#include "aarch64_process.h"

#include "aarch64_interpreter.h"
#include "elf.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t g_fallback_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int g_fallback_heap_lock_depth = 0;

static bool GuestAddressExecutable(Loader* loader, uint64_t addr) {
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
  return addr == 0 || GuestAddressExecutable(loader, addr);
}

static uint64_t LookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !GuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

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

void AARCH64ProcessRegisterGuestMemory(AARCH64ProcessRuntime* process,
                                       uint64_t start, size_t size) {
  if (process == NULL || !process->initialized || size == 0) {
    return;
  }
  AARCH64GuestMemoryRange* range = malloc(sizeof(*range));
  if (range == NULL) {
    return;
  }
  range->start = start;
  range->end = start + (uint64_t)size;
  pthread_rwlock_wrlock(&process->memory_lock);
  VectorAppend(&process->memory_ranges, range);
  pthread_rwlock_unlock(&process->memory_lock);
}

bool AARCH64ProcessGuestMemoryOk(AARCH64ProcessRuntime* process, uint64_t addr,
                                 size_t size) {
  if (process == NULL || !process->initialized) {
    return false;
  }
  pthread_rwlock_rdlock(&process->memory_lock);
  for (size_t i = 0; i < process->memory_ranges.length; i++) {
    AARCH64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    if (range != NULL && addr >= range->start && addr + size <= range->end) {
      pthread_rwlock_unlock(&process->memory_lock);
      return true;
    }
  }
  pthread_rwlock_unlock(&process->memory_lock);
  return false;
}

static void RegisterThreadMemory(AARCH64ProcessRuntime* process,
                                 AARCH64GuestThread* thread) {
  char* stack = thread->stack != NULL ? thread->stack : thread->cpu.stack;
  if (stack != NULL) {
    AARCH64ProcessRegisterGuestMemory(process, (uint64_t)(uintptr_t)stack,
                                      AARCH64_STACK_SIZE);
  }
  if (thread->tls_block != NULL && thread->tls_block_size > 0) {
    AARCH64ProcessRegisterGuestMemory(
        process, (uint64_t)(uintptr_t)thread->tls_block,
        thread->tls_block_size);
  }
}

static void GuestThreadDestruct(AARCH64GuestThread* thread) {
  if (thread == NULL) {
    return;
  }
  AARCH64InterpreterDestruct(&thread->cpu);
  if (thread->stack != NULL) {
    free(thread->stack);
    thread->stack = NULL;
  }
  if (thread->tls_block != NULL) {
    free(thread->tls_block);
    thread->tls_block = NULL;
  }
  free(thread);
}

void AARCH64InterpreterFail(AARCH64Interpreter* interpreter, int status) {
  if (interpreter != NULL && interpreter->guest_thread != NULL &&
      !interpreter->guest_thread->is_main) {
    interpreter->running = false;
    interpreter->pc = 0;
    AARCH64SyscallThreadExit(interpreter->guest_thread, status);
    return;
  }
  exit(status);
}

static void DestroyProcessLocks(AARCH64ProcessRuntime* process) {
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
  if (process->memory_mutex_initialized) {
    pthread_mutex_destroy(&process->memory_mutex);
    process->memory_mutex_initialized = false;
  }
}

static int RunWorkerGuestCalls(AARCH64GuestThread* thread) {
  if (thread->tls_init_fn != 0) {
    AARCH64InterpreterCall(&thread->cpu, thread->tls_init_fn, 0);
    if (thread->tls_fini_done) {
      return thread->cpu.exit_code;
    }
  }
  AARCH64InterpreterCall(&thread->cpu, thread->user_fn, thread->user_arg);
  int result = thread->cpu.exit_code;
  if (!thread->tls_fini_done && thread->tls_fini_fn != 0) {
    thread->tls_fini_done = true;
    AARCH64InterpreterCall(&thread->cpu, thread->tls_fini_fn, 0);
  }
  return result;
}

static void* GuestThreadHostEntry(void* arg) {
  AARCH64GuestThread* thread = (AARCH64GuestThread*)arg;
  AARCH64ProcessSetCurrentThread(thread->process, thread);

  pthread_mutex_lock(&thread->process->mutex);
  thread->state = kAARCH64GuestThreadRunning;
  pthread_mutex_unlock(&thread->process->mutex);

  int exit_code = RunWorkerGuestCalls(thread);

  pthread_mutex_lock(&thread->process->mutex);
  thread->exit_code = exit_code;
  thread->state = kAARCH64GuestThreadFinished;
  pthread_mutex_unlock(&thread->process->mutex);

  return NULL;
}

static AARCH64GuestThread* NewGuestThread(AARCH64ProcessRuntime* process,
                                          uint64_t tid, bool is_main) {
  AARCH64GuestThread* thread = calloc(1, sizeof(*thread));
  if (thread == NULL) {
    return NULL;
  }
  thread->process = process;
  thread->tid = tid;
  thread->is_main = is_main;
  thread->joinable = !is_main;
  thread->state = kAARCH64GuestThreadIdle;
  thread->exit_code = 0;
  return thread;
}

void AARCH64ProcessRuntimeInit(AARCH64ProcessRuntime* process,
                               struct AARCH64Runtime* runtime, Loader* loader) {
  memset(process, 0, sizeof(*process));
  process->runtime = runtime;
  process->loader = loader;
  if (pthread_mutex_init(&process->memory_mutex, NULL) != 0) {
    return;
  }
  process->memory_mutex_initialized = true;
  if (pthread_mutex_init(&process->mutex, NULL) != 0) {
    DestroyProcessLocks(process);
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
  VectorInit(&process->threads);
  VectorInit(&process->memory_ranges);
  process->next_tid = 1;
  process->initialized = true;

  if (loader->tls.present && loader->tls.block_size > 0) {
    AARCH64ProcessRegisterGuestMemory(process, loader->tls.tp_base,
                                      loader->tls.block_size);
  }
}

void AARCH64ProcessRuntimeDestruct(AARCH64ProcessRuntime* process) {
  if (!process->initialized) {
    DestroyProcessLocks(process);
    return;
  }

  pthread_mutex_lock(&process->mutex);
  process->shutting_down = true;
  pthread_mutex_unlock(&process->mutex);

  for (size_t i = 0; i < process->threads.length; i++) {
    AARCH64GuestThread* thread = process->threads.value.p[i];
    if (thread == NULL) {
      continue;
    }
    if (thread->host_thread_valid && !thread->is_main) {
      pthread_join(thread->host_thread, NULL);
      thread->host_thread_valid = false;
    }
    GuestThreadDestruct(thread);
  }
  VectorDestruct(&process->threads);

  for (size_t i = 0; i < process->memory_ranges.length; i++) {
    AARCH64GuestMemoryRange* range = process->memory_ranges.value.p[i];
    free(range);
  }
  VectorDestruct(&process->memory_ranges);

  DestroyProcessLocks(process);
  process->initialized = false;
}

AARCH64GuestThread* AARCH64ProcessGetCurrentThread(
    AARCH64ProcessRuntime* process) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  return (AARCH64GuestThread*)pthread_getspecific(process->current_thread_key);
}

void AARCH64ProcessSetCurrentThread(AARCH64ProcessRuntime* process,
                                    AARCH64GuestThread* thread) {
  pthread_setspecific(process->current_thread_key, thread);
}

AARCH64GuestThread* AARCH64ProcessFindThread(AARCH64ProcessRuntime* process,
                                             uint64_t tid) {
  if (process == NULL || tid == 0) {
    return NULL;
  }
  for (size_t i = 0; i < process->threads.length; i++) {
    AARCH64GuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->tid == tid) {
      return thread;
    }
  }
  return NULL;
}

AARCH64GuestThread* AARCH64ProcessCreateMainThread(
    AARCH64ProcessRuntime* process, uint64_t entry_address, int argc,
    char** argv, bool trace_registers, bool trace_instructions) {
  AARCH64GuestThread* thread =
      NewGuestThread(process, process->next_tid++, true);
  if (thread == NULL) {
    return NULL;
  }
  thread->host_thread = pthread_self();
  thread->host_thread_valid = true;
  thread->state = kAARCH64GuestThreadRunning;
  VectorAppend(&process->threads, thread);
  process->main_thread = thread;

  thread->tls_fini_fn =
      LookupGuestFunction(process->loader, "__davecc_tls_thread_fini");

  AARCH64InterpreterInitForThread(&thread->cpu, process, thread, process->loader,
                                  entry_address, argc, argv, NULL, 0, 0,
                                  trace_registers, trace_instructions);
  thread->cpu.guest_tid = thread->tid;
  RegisterThreadMemory(process, thread);
  AARCH64ProcessSetCurrentThread(process, thread);
  return thread;
}

static bool InitWorkerInterpreter(AARCH64GuestThread* thread) {
  thread->stack = malloc(AARCH64_STACK_SIZE);
  if (thread->stack == NULL) {
    return false;
  }

  uint64_t tp_base = 0;
  size_t tls_block_size = 0;
  if (thread->process->loader->tls.present) {
    if (!LoaderAllocThreadTlsBlock(thread->process->loader, &thread->tls_block,
                                   &thread->tls_block_size, &tp_base)) {
      return false;
    }
    tls_block_size = thread->tls_block_size;
  }

  AARCH64InterpreterInitForThread(
      &thread->cpu, thread->process, thread, thread->process->loader, 0, 0,
      NULL, thread->stack, tp_base, tls_block_size, false, false);
  thread->cpu.running = false;
  thread->cpu.pc = 0;
  return true;
}

int64_t AARCH64SyscallThreadCreate(AARCH64GuestThread* caller, uint64_t fn,
                                   uint64_t arg, uint64_t tls_init_fn,
                                   uint64_t tls_fini_fn) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  AARCH64ProcessRuntime* process = caller->process;
  Loader* loader = process->loader;

  if (!GuestAddressExecutable(loader, fn) ||
      !GuestExecutableOrZero(loader, tls_init_fn) ||
      !GuestExecutableOrZero(loader, tls_fini_fn)) {
    return -EINVAL;
  }

  AARCH64GuestThread* thread = NewGuestThread(process, 0, false);
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

  int rc = pthread_create(&thread->host_thread, NULL, GuestThreadHostEntry,
                          thread);
  if (rc != 0) {
    VectorPop(&process->threads);
    pthread_mutex_unlock(&process->mutex);
    GuestThreadDestruct(thread);
    return -ENOMEM;
  }
  thread->host_thread_valid = true;
  RegisterThreadMemory(process, thread);
  uint64_t tid = thread->tid;
  pthread_mutex_unlock(&process->mutex);
  return (int64_t)tid;
}

static bool GuestJoinResultPointerOk(AARCH64GuestThread* caller,
                                     uint64_t result_ptr) {
  if (result_ptr == 0) {
    return true;
  }
  AARCH64ProcessRuntime* process = caller->process;
  if (AARCH64ProcessGuestMemoryOk(process, result_ptr, sizeof(int))) {
    return true;
  }
  return GuestLoaderAddressOk(process->loader, result_ptr, sizeof(int));
}

int64_t AARCH64SyscallThreadJoin(AARCH64GuestThread* caller, uint64_t tid,
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

  AARCH64ProcessRuntime* process = caller->process;
  pthread_mutex_lock(&process->mutex);
  AARCH64GuestThread* thread = AARCH64ProcessFindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (!thread->joinable || thread->state == kAARCH64GuestThreadJoined) {
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
  pthread_t host_thread = thread->host_thread;
  pthread_mutex_unlock(&process->mutex);

  pthread_join(host_thread, NULL);

  pthread_mutex_lock(&process->mutex);
  thread->host_thread_valid = false;
  thread->state = kAARCH64GuestThreadJoined;
  int exit_code = thread->exit_code;
  pthread_mutex_unlock(&process->mutex);

  if (result_ptr != 0) {
    *(int*)(uintptr_t)result_ptr = exit_code;
  }
  return 0;
}

int64_t AARCH64SyscallThreadSelf(AARCH64GuestThread* caller) {
  if (caller == NULL) {
    return 0;
  }
  return (int64_t)caller->tid;
}

int64_t AARCH64SyscallGetTp(AARCH64GuestThread* caller) {
  if (caller == NULL || caller->cpu.tp_base == 0) {
    return 0;
  }
  return (int64_t)caller->cpu.tp_base;
}

void AARCH64SyscallThreadExit(AARCH64GuestThread* caller, int64_t status) {
  if (caller == NULL) {
    return;
  }
  if (!caller->tls_fini_done && caller->tls_fini_fn != 0) {
    caller->tls_fini_done = true;
    AARCH64InterpreterCall(&caller->cpu, caller->tls_fini_fn, 0);
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
  AARCH64InterpreterWriteX(&caller->cpu, 0, (uint64_t)(int)status);
  caller->cpu.running = false;
  caller->cpu.pc = 0;
}

int64_t AARCH64SyscallHeapLock(AARCH64GuestThread* caller) {
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

int64_t AARCH64SyscallHeapUnlock(AARCH64GuestThread* caller) {
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
