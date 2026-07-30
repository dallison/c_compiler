#include "risc_v_process.h"

#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t fallback_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int fallback_heap_lock_depth;

static void* ResolveGuestAddressThreadMapsLocked(RISCVProcessRuntime* process,
                                                 uint64_t addr, size_t size) {
  for (size_t i = 0; i < process->threads.length; i++) {
    RISCVGuestThread* thread = process->threads.value.p[i];
    RISCVInterpreter* cpu = thread != NULL ? thread->cpu : NULL;
    if (cpu == NULL) {
      continue;
    }
    uint64_t stack_start = cpu->stack_guest_base;
    if (cpu->stack != NULL && addr >= stack_start &&
        addr + size <= stack_start + RISC_V_STACK_SIZE) {
      return cpu->stack + (addr - stack_start);
    }
    uint64_t tls_start = cpu->tls_guest_base;
    if (cpu->tls_block != NULL && addr >= tls_start &&
        addr + size <= tls_start + cpu->tls_block_size) {
      return (char*)cpu->tls_block + (addr - tls_start);
    }
  }
  return NULL;
}

static void* ResolveGuestAddressFullLocked(RISCVProcessRuntime* process,
                                           uint64_t addr, size_t size) {
  if (process == NULL || !process->initialized || addr == 0 ||
      addr + size < addr) {
    return NULL;
  }
  void* mapped = ResolveGuestAddressThreadMapsLocked(process, addr, size);
  if (mapped != NULL) {
    return mapped;
  }
  Loader* loader = process->loader;
  if (loader == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr + size <= end) {
      return (void*)(uintptr_t)addr;
    }
  }
  return NULL;
}

static void WaitForGuestMapReaders(RISCVProcessRuntime* process) {
  pthread_mutex_lock(&process->mutex);
  pthread_mutex_unlock(&process->mutex);
}

static bool GuestReadMemory(RISCVProcessRuntime* process, uint64_t guest_addr,
                            void* buf, size_t size) {
  pthread_mutex_lock(&process->mutex);
  pthread_mutex_lock(&process->memory_mutex);
  void* host = ResolveGuestAddressFullLocked(process, guest_addr, size);
  if (host == NULL) {
    pthread_mutex_unlock(&process->memory_mutex);
    pthread_mutex_unlock(&process->mutex);
    return false;
  }
  memcpy(buf, host, size);
  pthread_mutex_unlock(&process->memory_mutex);
  pthread_mutex_unlock(&process->mutex);
  return true;
}

static bool GuestWriteMemory(RISCVProcessRuntime* process, uint64_t guest_addr,
                             const void* buf, size_t size) {
  pthread_mutex_lock(&process->mutex);
  pthread_mutex_lock(&process->memory_mutex);
  void* host = ResolveGuestAddressFullLocked(process, guest_addr, size);
  if (host == NULL) {
    pthread_mutex_unlock(&process->memory_mutex);
    pthread_mutex_unlock(&process->mutex);
    return false;
  }
  memcpy(host, buf, size);
  process->write_epoch++;
  pthread_mutex_unlock(&process->memory_mutex);
  pthread_mutex_unlock(&process->mutex);
  return true;
}

typedef struct {
  RISCVProcessRuntime* process;
} RISCVGuestAddrReadContext;

static bool RISCVGuestAddrRead(void* context, uint64_t guest_address,
                               void* value, size_t size) {
  RISCVGuestAddrReadContext* ctx = context;
  return GuestReadMemory(ctx->process, guest_address, value, size);
}

static void DestroyGuestThread(RISCVGuestThread* thread) {
  if (thread == NULL) {
    return;
  }
  if (!thread->is_main && thread->cpu != NULL) {
    RISCVInterpreterDestruct(thread->cpu);
  }
  free(thread->stack);
  free(thread->tls_block);
  free(thread);
}

static size_t CountActiveWorkersLocked(RISCVProcessRuntime* process) {
  size_t count = 0;
  for (size_t i = 0; i < process->threads.length; i++) {
    RISCVGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && !thread->is_main) {
      count++;
    }
  }
  return count;
}

static void ReapFinishedDetachedThreads(RISCVProcessRuntime* process) {
  for (;;) {
    RISCVGuestThread* batch[RISC_V_MAX_GUEST_THREADS];
    pthread_t hosts[RISC_V_MAX_GUEST_THREADS];
    size_t count = 0;

    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length && count < RISC_V_MAX_GUEST_THREADS;) {
      RISCVGuestThread* thread = process->threads.value.p[i];
      if (thread != NULL && !thread->is_main && thread->detached &&
          thread->host_thread_valid && !thread->join_in_progress &&
          thread->state == kRISCVGuestThreadFinished) {
        batch[count] = thread;
        hosts[count] = thread->host_thread;
        count++;
        VectorDeleteElement(&process->threads, i);
        continue;
      }
      i++;
    }
    pthread_mutex_unlock(&process->mutex);

    if (count == 0) {
      return;
    }

    for (size_t j = 0; j < count; j++) {
      pthread_join(hosts[j], NULL);
      WaitForGuestMapReaders(process);
      DestroyGuestThread(batch[j]);
    }
  }
}

static void WaitForActiveJoins(RISCVProcessRuntime* process) {
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

static void WaitForRunningWorkers(RISCVProcessRuntime* process) {
  for (;;) {
    ReapFinishedDetachedThreads(process);
    bool pending = false;
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      RISCVGuestThread* thread = process->threads.value.p[i];
      if (thread != NULL && !thread->is_main && thread->host_thread_valid &&
          thread->state != kRISCVGuestThreadFinished) {
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

static uint64_t GuestFunctionRuntime(Loader* loader, uint64_t addr) {
  if (RISCVGuestAddressExecutable(loader, addr)) {
    return addr;
  }
  uint64_t runtime = 0;
  if (LoaderLinkedAddressToRuntime(loader, NULL, addr, &runtime) &&
      RISCVGuestAddressExecutable(loader, runtime)) {
    return runtime;
  }
  return 0;
}

static RISCVGuestThread* NewGuestThread(RISCVProcessRuntime* process,
                                        uint64_t tid, bool is_main) {
  RISCVGuestThread* thread = calloc(1, sizeof(*thread));
  if (thread == NULL) {
    return NULL;
  }
  thread->process = process;
  thread->tid = tid;
  thread->is_main = is_main;
  thread->joinable = !is_main;
  thread->state = kRISCVGuestThreadIdle;
  thread->cpu = &thread->cpu_storage;
  return thread;
}

bool RISCVProcessRuntimeInit(RISCVProcessRuntime* process, Loader* loader) {
  memset(process, 0, sizeof(*process));
  process->loader = loader;
  if (pthread_mutex_init(&process->memory_mutex, NULL) != 0) {
    return false;
  }
  if (pthread_mutex_init(&process->mutex, NULL) != 0) {
    pthread_mutex_destroy(&process->memory_mutex);
    return false;
  }
  if (pthread_mutex_init(&process->got_resolve_mutex, NULL) != 0) {
    pthread_mutex_destroy(&process->mutex);
    pthread_mutex_destroy(&process->memory_mutex);
    return false;
  }
  if (pthread_mutex_init(&process->heap_mutex, NULL) != 0) {
    pthread_mutex_destroy(&process->got_resolve_mutex);
    pthread_mutex_destroy(&process->mutex);
    pthread_mutex_destroy(&process->memory_mutex);
    return false;
  }
  if (pthread_key_create(&process->current_thread_key, NULL) != 0) {
    pthread_mutex_destroy(&process->heap_mutex);
    pthread_mutex_destroy(&process->got_resolve_mutex);
    pthread_mutex_destroy(&process->mutex);
    pthread_mutex_destroy(&process->memory_mutex);
    return false;
  }
  VectorInit(&process->threads);
  process->next_tid = 2;
  if (!GuestAddrWaitTableInit(&process->addr_wait_table)) {
    VectorDestruct(&process->threads);
    pthread_key_delete(process->current_thread_key);
    pthread_mutex_destroy(&process->heap_mutex);
    pthread_mutex_destroy(&process->got_resolve_mutex);
    pthread_mutex_destroy(&process->mutex);
    pthread_mutex_destroy(&process->memory_mutex);
    return false;
  }
  process->initialized = true;
  return true;
}

void RISCVProcessRuntimeDestruct(RISCVProcessRuntime* process) {
  if (!process->initialized) {
    return;
  }
  pthread_mutex_lock(&process->mutex);
  process->shutting_down = true;
  pthread_mutex_unlock(&process->mutex);
  GuestAddrWaitTableShutdown(&process->addr_wait_table);
  WaitForRunningWorkers(process);
  WaitForActiveJoins(process);
  ReapFinishedDetachedThreads(process);

  RISCVGuestThread* workers[RISC_V_MAX_GUEST_THREADS];
  pthread_t worker_hosts[RISC_V_MAX_GUEST_THREADS];
  size_t worker_count = 0;
  RISCVGuestThread* main_thread = NULL;

  pthread_mutex_lock(&process->mutex);
  for (size_t i = 0; i < process->threads.length; i++) {
    RISCVGuestThread* thread = process->threads.value.p[i];
    if (thread == NULL) {
      continue;
    }
    if (thread->is_main) {
      main_thread = thread;
      continue;
    }
    if (thread->host_thread_valid && !thread->join_in_progress &&
        worker_count < RISC_V_MAX_GUEST_THREADS) {
      workers[worker_count] = thread;
      worker_hosts[worker_count] = thread->host_thread;
      worker_count++;
    }
  }
  VectorClear(&process->threads);
  if (main_thread != NULL) {
    VectorAppend(&process->threads, main_thread);
  }
  pthread_mutex_unlock(&process->mutex);

  for (size_t i = 0; i < worker_count; i++) {
    pthread_join(worker_hosts[i], NULL);
    WaitForGuestMapReaders(process);
    DestroyGuestThread(workers[i]);
  }

  if (main_thread != NULL) {
    WaitForGuestMapReaders(process);
    DestroyGuestThread(main_thread);
  }
  VectorDestruct(&process->threads);
  GuestAddrWaitTableDestruct(&process->addr_wait_table);
  pthread_key_delete(process->current_thread_key);
  pthread_mutex_destroy(&process->heap_mutex);
  pthread_mutex_destroy(&process->got_resolve_mutex);
  pthread_mutex_destroy(&process->mutex);
  pthread_mutex_destroy(&process->memory_mutex);
  process->initialized = false;
}

RISCVGuestThread* RISCVProcessAttachMainThread(RISCVProcessRuntime* process,
                                               RISCVInterpreter* cpu) {
  RISCVGuestThread* thread = NewGuestThread(process, 1, true);
  if (thread == NULL) {
    return NULL;
  }
  thread->cpu = cpu;
  thread->host_thread = pthread_self();
  thread->host_thread_valid = true;
  thread->state = kRISCVGuestThreadRunning;
  thread->tls_fini_fn =
      RISCVLookupGuestFunction(process->loader, "__davecc_tls_thread_fini");
  cpu->process = process;
  cpu->guest_thread = thread;
  process->main_thread = thread;
  VectorAppend(&process->threads, thread);
  pthread_setspecific(process->current_thread_key, thread);
  return thread;
}

RISCVGuestThread* RISCVProcessGetCurrentThread(RISCVProcessRuntime* process) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  return pthread_getspecific(process->current_thread_key);
}

void* RISCVProcessResolveGuestAddress(RISCVProcessRuntime* process,
                                      uint64_t addr, size_t size) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  pthread_mutex_lock(&process->mutex);
  void* result = ResolveGuestAddressFullLocked(process, addr, size);
  pthread_mutex_unlock(&process->mutex);
  return result;
}

static int RunWorker(RISCVGuestThread* thread) {
  if (thread->tls_init_fn != 0) {
    RISCVInterpreterCallWithArg(thread->cpu, thread->tls_init_fn, 0);
    if (thread->tls_fini_done) {
      return thread->cpu->exit_code;
    }
  }
  int result = RISCVInterpreterCallWithArg(thread->cpu, thread->user_fn,
                                           thread->user_arg);
  if (!thread->tls_fini_done && thread->tls_fini_fn != 0) {
    thread->tls_fini_done = true;
    RISCVInterpreterCall(thread->cpu, thread->tls_fini_fn);
  }
  return result;
}

static void* WorkerEntry(void* arg) {
  RISCVGuestThread* thread = arg;
  pthread_setspecific(thread->process->current_thread_key, thread);
  pthread_mutex_lock(&thread->process->mutex);
  thread->state = kRISCVGuestThreadRunning;
  pthread_mutex_unlock(&thread->process->mutex);
  int result = RunWorker(thread);
  if (thread->heap_lock_depth > 0) {
    thread->heap_lock_depth = 0;
    pthread_mutex_unlock(&thread->process->heap_mutex);
  }
  pthread_mutex_lock(&thread->process->mutex);
  thread->exit_code = result;
  thread->state = kRISCVGuestThreadFinished;
  pthread_mutex_unlock(&thread->process->mutex);
  return NULL;
}

static bool InitWorker(RISCVGuestThread* thread) {
  thread->stack = malloc(RISC_V_STACK_SIZE);
  if (thread->stack == NULL) {
    return false;
  }
  if (thread->process->loader->tls.present) {
    uint64_t tp_base;
    if (!LoaderAllocThreadTlsBlock(thread->process->loader, &thread->tls_block,
                                   &thread->tls_block_size, &tp_base)) {
      return false;
    }
  }
  RISCVInterpreterInitForThread(
      thread->cpu, thread->process, thread, thread->process->loader, 0, 0,
      NULL, thread->stack, thread->tls_block, thread->tls_block_size, false,
      false);
  return true;
}

int64_t RISCVSyscallThreadCreate(RISCVGuestThread* caller, uint64_t fn,
                                 uint64_t arg, uint64_t tls_init_fn,
                                 uint64_t tls_fini_fn) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  RISCVProcessRuntime* process = caller->process;
  uint64_t runtime_fn = GuestFunctionRuntime(process->loader, fn);
  uint64_t runtime_tls_init =
      tls_init_fn == 0 ? 0
                       : GuestFunctionRuntime(process->loader, tls_init_fn);
  uint64_t runtime_tls_fini =
      tls_fini_fn == 0 ? 0
                       : GuestFunctionRuntime(process->loader, tls_fini_fn);
  if (runtime_fn == 0 || (tls_init_fn != 0 && runtime_tls_init == 0) ||
      (tls_fini_fn != 0 && runtime_tls_fini == 0)) {
    return -EINVAL;
  }
  RISCVGuestThread* thread = NewGuestThread(process, 0, false);
  if (thread == NULL) {
    return -ENOMEM;
  }
  thread->user_fn = runtime_fn;
  thread->user_arg = arg;
  thread->tls_init_fn = runtime_tls_init;
  thread->tls_fini_fn = runtime_tls_fini;

  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  if (process->shutting_down) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EINVAL;
  }
  if (CountActiveWorkersLocked(process) >= RISC_V_MAX_GUEST_THREADS - 1) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EAGAIN;
  }
  thread->tid = process->next_tid++;
  VectorAppend(&process->threads, thread);
  pthread_mutex_unlock(&process->mutex);

  if (!InitWorker(thread)) {
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      if (process->threads.value.p[i] == thread) {
        VectorDeleteElement(&process->threads, i);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    WaitForGuestMapReaders(process);
    DestroyGuestThread(thread);
    return -ENOMEM;
  }

  pthread_mutex_lock(&process->mutex);
  if (process->shutting_down) {
    for (size_t i = 0; i < process->threads.length; i++) {
      if (process->threads.value.p[i] == thread) {
        VectorDeleteElement(&process->threads, i);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    WaitForGuestMapReaders(process);
    DestroyGuestThread(thread);
    return -EINVAL;
  }
  int rc = pthread_create(&thread->host_thread, NULL, WorkerEntry, thread);
  if (rc != 0) {
    for (size_t i = 0; i < process->threads.length; i++) {
      if (process->threads.value.p[i] == thread) {
        VectorDeleteElement(&process->threads, i);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    WaitForGuestMapReaders(process);
    DestroyGuestThread(thread);
    return -ENOMEM;
  }
  thread->host_thread_valid = true;
  uint64_t public_tid = thread->tid;
  pthread_mutex_unlock(&process->mutex);
  return (int64_t)public_tid;
}

static RISCVGuestThread* FindThread(RISCVProcessRuntime* process,
                                    uint64_t tid) {
  for (size_t i = 0; i < process->threads.length; i++) {
    RISCVGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->tid == tid) {
      return thread;
    }
  }
  return NULL;
}

int64_t RISCVSyscallThreadJoin(RISCVGuestThread* caller, uint64_t tid,
                               uint64_t result_ptr) {
  if (caller == NULL || caller->process == NULL || tid == 0 ||
      tid == caller->tid) {
    return -EINVAL;
  }
  if (result_ptr != 0) {
    unsigned char probe;
    if (!GuestReadMemory(caller->process, result_ptr, &probe, 1)) {
      return -EINVAL;
    }
  }
  RISCVProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  RISCVGuestThread* thread = FindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (!thread->joinable || thread->detached || !thread->host_thread_valid ||
      thread->is_main) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  thread->joinable = false;
  thread->join_in_progress = true;
  process->active_joins++;
  pthread_t host = thread->host_thread;
  pthread_mutex_unlock(&process->mutex);

  pthread_join(host, NULL);
  int result = thread->exit_code;
  if (result_ptr != 0 &&
      !GuestWriteMemory(process, result_ptr, &result, sizeof(result))) {
    pthread_mutex_lock(&process->mutex);
    thread->host_thread_valid = false;
    thread->join_in_progress = false;
    process->active_joins--;
    for (size_t i = 0; i < process->threads.length; i++) {
      if (process->threads.value.p[i] == thread) {
        VectorDeleteElement(&process->threads, i);
        break;
      }
    }
    pthread_mutex_unlock(&process->mutex);
    WaitForGuestMapReaders(process);
    DestroyGuestThread(thread);
    return -EINVAL;
  }

  pthread_mutex_lock(&process->mutex);
  thread->host_thread_valid = false;
  thread->join_in_progress = false;
  process->active_joins--;
  for (size_t i = 0; i < process->threads.length; i++) {
    if (process->threads.value.p[i] == thread) {
      VectorDeleteElement(&process->threads, i);
      break;
    }
  }
  pthread_mutex_unlock(&process->mutex);
  WaitForGuestMapReaders(process);
  DestroyGuestThread(thread);
  return 0;
}

int64_t RISCVSyscallThreadSelf(RISCVGuestThread* caller) {
  return caller == NULL ? 0 : (int64_t)caller->tid;
}

int64_t RISCVSyscallGetTp(RISCVGuestThread* caller) {
  return caller == NULL || caller->cpu == NULL
             ? 0
             : caller->cpu->iregs[RISC_V_REG_tp];
}

void RISCVSyscallThreadExit(RISCVGuestThread* caller, int64_t status) {
  if (caller == NULL) {
    return;
  }
  if (!caller->tls_fini_done && caller->tls_fini_fn != 0) {
    caller->tls_fini_done = true;
    RISCVInterpreterCall(caller->cpu, caller->tls_fini_fn);
  }
  if (caller->heap_lock_depth > 0) {
    caller->heap_lock_depth = 0;
    pthread_mutex_unlock(&caller->process->heap_mutex);
  }
  if (caller->is_main) {
    exit((int)status);
  }
  caller->exit_code = (int)status;
  caller->cpu->exit_code = (int)status;
  caller->cpu->iregs[RISC_V_REG_a0] = status;
  caller->cpu->running = false;
  caller->cpu->pc = 0;
}

int64_t RISCVSyscallHeapLock(RISCVGuestThread* caller) {
  if (caller != NULL && caller->process != NULL) {
    if (caller->heap_lock_depth++ == 0) {
      pthread_mutex_lock(&caller->process->heap_mutex);
    }
    return 0;
  }
  if (fallback_heap_lock_depth++ == 0) {
    pthread_mutex_lock(&fallback_heap_mutex);
  }
  return 0;
}

int64_t RISCVSyscallHeapUnlock(RISCVGuestThread* caller) {
  if (caller != NULL && caller->process != NULL) {
    if (caller->heap_lock_depth <= 0) {
      return -EINVAL;
    }
    if (--caller->heap_lock_depth == 0) {
      pthread_mutex_unlock(&caller->process->heap_mutex);
    }
    return 0;
  }
  if (fallback_heap_lock_depth <= 0) {
    return -EINVAL;
  }
  if (--fallback_heap_lock_depth == 0) {
    pthread_mutex_unlock(&fallback_heap_mutex);
  }
  return 0;
}

int64_t RISCVSyscallTime(void) {
  return (int64_t)time(NULL);
}

int64_t RISCVSyscallClock(void) {
  return (int64_t)clock();
}

int64_t RISCVSyscallThreadDetach(RISCVGuestThread* caller, uint64_t tid) {
  if (caller == NULL || caller->process == NULL || tid == 0 ||
      tid == (uint64_t)caller->tid) {
    return -EINVAL;
  }
  RISCVProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  RISCVGuestThread* thread = FindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (!thread->joinable || thread->detached || thread->is_main ||
      !thread->host_thread_valid) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  thread->joinable = false;
  thread->detached = true;
  pthread_mutex_unlock(&process->mutex);

  ReapFinishedDetachedThreads(process);
  return 0;
}

int64_t RISCVSyscallAddrWait(RISCVGuestThread* caller, uint64_t address,
                             uint64_t expected_ptr, uint64_t size,
                             int64_t timeout_us) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  if (size != 1 && size != 2 && size != 4 && size != 8) {
    return -EINVAL;
  }
  if (timeout_us < -1) {
    return -EINVAL;
  }
  RISCVProcessRuntime* process = caller->process;
  unsigned char expected[8];
  if (!GuestReadMemory(process, expected_ptr, expected, size)) {
    return -EINVAL;
  }

  RISCVGuestAddrReadContext ctx = {.process = process};
  int wait_result =
      GuestAddrWait(&process->addr_wait_table, RISCVGuestAddrRead, &ctx, address,
                    expected, size, timeout_us);
  switch (wait_result) {
    case kGuestAddrWaitSuccess:
      return 0;
    case kGuestAddrWaitTimedOut:
      return 1;
    default:
      return -1;
  }
}

int64_t RISCVSyscallAddrWake(RISCVGuestThread* caller, uint64_t address,
                             int64_t wake_all) {
  if (caller == NULL || caller->process == NULL) {
    return -1;
  }
  RISCVProcessRuntime* process = caller->process;
  int wake_result = GuestAddrWake(&process->addr_wait_table, address,
                                  wake_all != 0);
  return wake_result >= 0 ? wake_result : -1;
}

int64_t RISCVSyscallThreadSleep(RISCVGuestThread* caller, uint64_t duration_ptr,
                                uint64_t remaining_ptr) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  RISCVProcessRuntime* process = caller->process;
  struct timespec duration;
  if (!GuestReadMemory(process, duration_ptr, &duration, sizeof(duration))) {
    return -EINVAL;
  }
  if (duration.tv_sec < 0 || duration.tv_nsec < 0 ||
      duration.tv_nsec >= 1000000000) {
    return -EINVAL;
  }

  struct timespec remaining = {0};
  struct timespec* remaining_out = remaining_ptr != 0 ? &remaining : NULL;
  if (nanosleep(&duration, remaining_out) != 0) {
    if (remaining_ptr != 0 && remaining_out != NULL) {
      (void)GuestWriteMemory(process, remaining_ptr, &remaining,
                             sizeof(remaining));
    }
    return -errno;
  }
  if (remaining_ptr != 0) {
    if (!GuestWriteMemory(process, remaining_ptr, &remaining,
                          sizeof(remaining))) {
      return -EINVAL;
    }
  }
  return 0;
}

int64_t RISCVSyscallHardwareConcurrency(void) {
  long count = sysconf(_SC_NPROCESSORS_ONLN);
  if (count < 1) {
    count = 1;
  }
  if (count > RISC_V_MAX_GUEST_THREADS) {
    count = RISC_V_MAX_GUEST_THREADS;
  }
  return count;
}
