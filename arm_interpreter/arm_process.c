#include "arm_process.h"

#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t fallback_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int fallback_heap_lock_depth;

static int64_t SegmentFileOffsetDelta(const ELFProgramHeader* segment) {
  int64_t page_size = sysconf(_SC_PAGESIZE);
  return segment->offset - (segment->offset & ~(page_size - 1));
}

static bool GuestAddressOk(Loader* loader, uint64_t addr, size_t size) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      uint64_t start = (uint64_t)(uintptr_t)region->address;
      uint64_t end = start + (uint64_t)region->length;
      if (addr >= start && addr + size <= end) {
        return true;
      }
      continue;
    }
    ELFProgramHeader* segment = region->segment;
    uint64_t base = (uint64_t)(uintptr_t)region->address +
                    (uint64_t)SegmentFileOffsetDelta(segment);
    if (addr >= base && addr + size <= base + (uint64_t)region->length) {
      return true;
    }
  }
  return false;
}

static void* ResolveGuestAddressThreadMapsLocked(ARMProcessRuntime* process,
                                                 uint64_t addr, size_t size) {
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    ARMInterpreter* cpu = thread != NULL ? thread->cpu : NULL;
    if (cpu == NULL) {
      continue;
    }
    uint64_t stack_start = cpu->stack_guest_base;
    if (cpu->stack != NULL && addr >= stack_start &&
        addr + size <= stack_start + ARM_STACK_SIZE) {
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

static void* ResolveGuestAddressFullLocked(ARMProcessRuntime* process,
                                           uint64_t addr, size_t size) {
  if (process == NULL || !process->initialized || addr + size < addr) {
    return NULL;
  }
  void* mapped = ResolveGuestAddressThreadMapsLocked(process, addr, size);
  if (mapped != NULL) {
    return mapped;
  }
  if ((addr >> 32) != 0) {
    mapped = ResolveGuestAddressThreadMapsLocked(process, addr & 0xffffffffu,
                                                 size);
    if (mapped != NULL) {
      return mapped;
    }
    addr &= 0xffffffffu;
  }
  Loader* loader = process->loader;
  if (loader == NULL) {
    return NULL;
  }
  if (GuestAddressOk(loader, addr, size)) {
    return (void*)(uintptr_t)addr;
  }
  uint64_t host = 0;
  if (LoaderLinkedAddressToRuntime(loader, NULL, addr, &host) &&
      GuestAddressOk(loader, host, size)) {
    return (void*)(uintptr_t)host;
  }
  return NULL;
}

static void WaitForGuestMapReaders(ARMProcessRuntime* process) {
  pthread_mutex_lock(&process->mutex);
  pthread_mutex_unlock(&process->mutex);
}

static bool GuestReadMemory(ARMProcessRuntime* process, uint64_t guest_addr,
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

static bool GuestWriteMemory(ARMProcessRuntime* process, uint64_t guest_addr,
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
  ARMProcessRuntime* process;
} ARMGuestAddrReadContext;

static bool ARMGuestAddrRead(void* context, uint64_t guest_address,
                             void* value, size_t size) {
  ARMGuestAddrReadContext* ctx = context;
  return GuestReadMemory(ctx->process, guest_address, value, size);
}

static void DestroyGuestThread(ARMGuestThread* thread) {
  if (thread == NULL) {
    return;
  }
  if (!thread->is_main && thread->cpu != NULL) {
    ARMInterpreterDestruct(thread->cpu);
  }
  free(thread->stack);
  free(thread->tls_block);
  free(thread);
}

static bool IsWorkerSlotInUseLocked(ARMProcessRuntime* process, uint32_t slot) {
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && !thread->is_main && thread->slot == slot) {
      return true;
    }
  }
  return false;
}

static bool ReserveWorkerSlotAndTidLocked(ARMProcessRuntime* process,
                                          ARMGuestThread* thread) {
  for (uint32_t slot = 1; slot < ARM_MAX_GUEST_THREADS; slot++) {
    if (!IsWorkerSlotInUseLocked(process, slot)) {
      thread->slot = slot;
      thread->tid = process->next_tid++;
      return true;
    }
  }
  return false;
}

static size_t CountActiveWorkersLocked(ARMProcessRuntime* process) {
  size_t count = 0;
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && !thread->is_main) {
      count++;
    }
  }
  return count;
}

static void ReapFinishedDetachedThreads(ARMProcessRuntime* process) {
  for (;;) {
    ARMGuestThread* batch[ARM_MAX_GUEST_THREADS];
    pthread_t hosts[ARM_MAX_GUEST_THREADS];
    size_t count = 0;

    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length && count < ARM_MAX_GUEST_THREADS;) {
      ARMGuestThread* thread = process->threads.value.p[i];
      if (thread != NULL && !thread->is_main && thread->detached &&
          thread->host_thread_valid && !thread->join_in_progress &&
          thread->state == kARMGuestThreadFinished) {
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

static void WaitForActiveJoins(ARMProcessRuntime* process) {
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

static void WaitForRunningWorkers(ARMProcessRuntime* process) {
  for (;;) {
    ReapFinishedDetachedThreads(process);
    bool pending = false;
    pthread_mutex_lock(&process->mutex);
    for (size_t i = 0; i < process->threads.length; i++) {
      ARMGuestThread* thread = process->threads.value.p[i];
      if (thread != NULL && !thread->is_main && thread->host_thread_valid &&
          thread->state != kARMGuestThreadFinished) {
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

static ARMGuestThread* NewGuestThread(ARMProcessRuntime* process,
                                      uint32_t tid, bool is_main) {
  ARMGuestThread* thread = calloc(1, sizeof(*thread));
  if (thread == NULL) {
    return NULL;
  }
  thread->process = process;
  thread->tid = tid;
  thread->is_main = is_main;
  thread->joinable = !is_main;
  thread->state = kARMGuestThreadIdle;
  thread->cpu = &thread->cpu_storage;
  return thread;
}

bool ARMProcessRuntimeInit(ARMProcessRuntime* process, Loader* loader) {
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

void ARMProcessRuntimeDestruct(ARMProcessRuntime* process) {
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

  ARMGuestThread* workers[ARM_MAX_GUEST_THREADS];
  pthread_t worker_hosts[ARM_MAX_GUEST_THREADS];
  size_t worker_count = 0;
  ARMGuestThread* main_thread = NULL;

  pthread_mutex_lock(&process->mutex);
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    if (thread == NULL) {
      continue;
    }
    if (thread->is_main) {
      main_thread = thread;
      continue;
    }
    if (thread->host_thread_valid && !thread->join_in_progress &&
        worker_count < ARM_MAX_GUEST_THREADS) {
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

ARMGuestThread* ARMProcessAttachMainThread(ARMProcessRuntime* process,
                                           ARMInterpreter* cpu) {
  ARMGuestThread* thread = NewGuestThread(process, 1, true);
  if (thread == NULL) {
    return NULL;
  }
  thread->tid = 1;
  thread->slot = 0;
  thread->cpu = cpu;
  thread->host_thread = pthread_self();
  thread->host_thread_valid = true;
  thread->state = kARMGuestThreadRunning;
  thread->tls_fini_fn =
      ARMLookupGuestFunction(process->loader, "__davecc_tls_thread_fini");
  cpu->process = process;
  cpu->guest_thread = thread;
  process->main_thread = thread;
  VectorAppend(&process->threads, thread);
  pthread_setspecific(process->current_thread_key, thread);
  return thread;
}

ARMGuestThread* ARMProcessGetCurrentThread(ARMProcessRuntime* process) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  return pthread_getspecific(process->current_thread_key);
}

void* ARMProcessResolveGuestAddress(ARMProcessRuntime* process, uint64_t addr,
                                    size_t size) {
  if (process == NULL || !process->initialized) {
    return NULL;
  }
  pthread_mutex_lock(&process->mutex);
  void* result = ResolveGuestAddressFullLocked(process, addr, size);
  pthread_mutex_unlock(&process->mutex);
  return result;
}

static int RunWorker(ARMGuestThread* thread) {
  if (thread->tls_init_fn != 0) {
    ARMInterpreterCall(thread->cpu, thread->tls_init_fn);
    if (thread->tls_fini_done) {
      return thread->cpu->exit_code;
    }
  }
  int result =
      ARMInterpreterCallWithArg(thread->cpu, thread->user_fn, thread->user_arg);
  if (!thread->tls_fini_done && thread->tls_fini_fn != 0) {
    thread->tls_fini_done = true;
    ARMInterpreterCall(thread->cpu, thread->tls_fini_fn);
  }
  return result;
}

static void* WorkerEntry(void* arg) {
  ARMGuestThread* thread = arg;
  pthread_setspecific(thread->process->current_thread_key, thread);
  pthread_mutex_lock(&thread->process->mutex);
  thread->state = kARMGuestThreadRunning;
  pthread_mutex_unlock(&thread->process->mutex);
  int result = RunWorker(thread);
  if (thread->heap_lock_depth > 0) {
    thread->heap_lock_depth = 0;
    pthread_mutex_unlock(&thread->process->heap_mutex);
  }
  pthread_mutex_lock(&thread->process->mutex);
  thread->exit_code = result;
  thread->state = kARMGuestThreadFinished;
  pthread_mutex_unlock(&thread->process->mutex);
  return NULL;
}

static bool InitWorker(ARMGuestThread* thread) {
  thread->stack = malloc(ARM_STACK_SIZE);
  if (thread->stack == NULL) {
    return false;
  }
  if (thread->process->loader->tls.present) {
    uint64_t tp_base;
    if (!LoaderAllocThreadTlsBlock(thread->process->loader,
                                   &thread->tls_block,
                                   &thread->tls_block_size, &tp_base)) {
      return false;
    }
  }
  ARMInterpreterInitForThread(
      thread->cpu, thread->process, thread, thread->process->loader, 0, 0,
      NULL, thread->stack, thread->tls_block, thread->tls_block_size, false,
      false);
  return true;
}

int32_t ARMSyscallThreadCreate(ARMGuestThread* caller, uint32_t fn,
                               uint32_t arg, uint32_t tls_init_fn,
                               uint32_t tls_fini_fn) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  ARMProcessRuntime* process = caller->process;
  uint64_t runtime_fn = ARMGuestFunctionRuntime(process->loader, fn);
  uint64_t runtime_tls_init =
      tls_init_fn == 0 ? 0
                       : ARMGuestFunctionRuntime(process->loader, tls_init_fn);
  uint64_t runtime_tls_fini =
      tls_fini_fn == 0 ? 0
                       : ARMGuestFunctionRuntime(process->loader, tls_fini_fn);
  if (runtime_fn == 0 || (tls_init_fn != 0 && runtime_tls_init == 0) ||
      (tls_fini_fn != 0 && runtime_tls_fini == 0)) {
    return -EINVAL;
  }
  ARMGuestThread* thread = NewGuestThread(process, 0, false);
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
  if (CountActiveWorkersLocked(process) >= ARM_MAX_GUEST_THREADS - 1) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EAGAIN;
  }
  if (!ReserveWorkerSlotAndTidLocked(process, thread)) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EAGAIN;
  }
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
  uint32_t public_tid = thread->tid;
  pthread_mutex_unlock(&process->mutex);
  return (int32_t)public_tid;
}

static ARMGuestThread* FindThread(ARMProcessRuntime* process, uint32_t tid) {
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->tid == tid) {
      return thread;
    }
  }
  return NULL;
}

int32_t ARMSyscallThreadJoin(ARMGuestThread* caller, uint32_t tid,
                             uint32_t result_ptr) {
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
  ARMProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  ARMGuestThread* thread = FindThread(process, tid);
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

int32_t ARMSyscallThreadSelf(ARMGuestThread* caller) {
  return caller == NULL ? 0 : (int32_t)caller->tid;
}

int32_t ARMSyscallGetTp(ARMGuestThread* caller) {
  return caller == NULL || caller->cpu == NULL
             ? 0
             : (int32_t)caller->cpu->tp_base;
}

void ARMSyscallThreadExit(ARMGuestThread* caller, int32_t status) {
  if (caller == NULL) {
    return;
  }
  if (!caller->tls_fini_done && caller->tls_fini_fn != 0) {
    caller->tls_fini_done = true;
    ARMInterpreterCall(caller->cpu, caller->tls_fini_fn);
  }
  if (caller->heap_lock_depth > 0) {
    caller->heap_lock_depth = 0;
    pthread_mutex_unlock(&caller->process->heap_mutex);
  }
  if (caller->is_main) {
    exit(status);
  }
  caller->exit_code = status;
  caller->cpu->exit_code = status;
  caller->cpu->regs[0] = (uint32_t)status;
  caller->cpu->running = false;
  caller->cpu->pc = 0;
}

int32_t ARMSyscallThreadDetach(ARMGuestThread* caller, uint32_t tid) {
  if (caller == NULL || caller->process == NULL || tid == 0 ||
      tid == caller->tid) {
    return -EINVAL;
  }
  ARMProcessRuntime* process = caller->process;
  ReapFinishedDetachedThreads(process);

  pthread_mutex_lock(&process->mutex);
  ARMGuestThread* thread = FindThread(process, tid);
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

int32_t ARMSyscallAddrWait(ARMGuestThread* caller, uint32_t address,
                           uint32_t expected_ptr, uint32_t size,
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
  ARMProcessRuntime* process = caller->process;
  unsigned char expected[8];
  if (!GuestReadMemory(process, expected_ptr, expected, size)) {
    return -EINVAL;
  }

  ARMGuestAddrReadContext ctx = {.process = process};
  int wait_result =
      GuestAddrWait(&process->addr_wait_table, ARMGuestAddrRead, &ctx, address,
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

int32_t ARMSyscallAddrWake(ARMGuestThread* caller, uint32_t address,
                           uint32_t wake_all) {
  if (caller == NULL || caller->process == NULL) {
    return -1;
  }
  ARMProcessRuntime* process = caller->process;
  int wake_result = GuestAddrWake(&process->addr_wait_table, address,
                                  wake_all != 0);
  return wake_result >= 0 ? wake_result : -1;
}

int32_t ARMSyscallThreadSleep(ARMGuestThread* caller, uint32_t duration_ptr,
                              uint32_t remaining_ptr) {
  if (caller == NULL || caller->process == NULL) {
    return -EINVAL;
  }
  ARMProcessRuntime* process = caller->process;
  typedef struct {
    int32_t tv_sec;
    int32_t tv_nsec;
  } ARMGuestTimespec;
  ARMGuestTimespec guest_duration;
  if (!GuestReadMemory(process, duration_ptr, &guest_duration,
                       sizeof(guest_duration))) {
    return -EINVAL;
  }
  struct timespec duration = {
      .tv_sec = guest_duration.tv_sec,
      .tv_nsec = guest_duration.tv_nsec,
  };
  if (duration.tv_sec < 0 || duration.tv_nsec < 0 ||
      duration.tv_nsec >= 1000000000) {
    return -EINVAL;
  }

  struct timespec remaining = {0};
  struct timespec* remaining_out = remaining_ptr != 0 ? &remaining : NULL;
  if (nanosleep(&duration, remaining_out) != 0) {
    if (remaining_ptr != 0 && remaining_out != NULL) {
      ARMGuestTimespec guest_remaining = {
          .tv_sec = (int32_t)remaining.tv_sec,
          .tv_nsec = (int32_t)remaining.tv_nsec,
      };
      (void)GuestWriteMemory(process, remaining_ptr, &guest_remaining,
                             sizeof(guest_remaining));
    }
    return -errno;
  }
  if (remaining_ptr != 0) {
    ARMGuestTimespec guest_remaining = {
        .tv_sec = (int32_t)remaining.tv_sec,
        .tv_nsec = (int32_t)remaining.tv_nsec,
    };
    if (!GuestWriteMemory(process, remaining_ptr, &guest_remaining,
                          sizeof(guest_remaining))) {
      return -EINVAL;
    }
  }
  return 0;
}

int32_t ARMSyscallHardwareConcurrency(void) {
  long count = sysconf(_SC_NPROCESSORS_ONLN);
  if (count < 1) {
    count = 1;
  }
  if (count > ARM_MAX_GUEST_THREADS) {
    count = ARM_MAX_GUEST_THREADS;
  }
  return (int32_t)count;
}

int32_t ARMSyscallHeapLock(ARMGuestThread* caller) {
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

int32_t ARMSyscallHeapUnlock(ARMGuestThread* caller) {
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
