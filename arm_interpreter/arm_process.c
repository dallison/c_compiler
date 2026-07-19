#include "arm_process.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t fallback_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int fallback_heap_lock_depth;

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
  process->next_tid = 1;
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
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    if (thread != NULL && thread->host_thread_valid && !thread->is_main) {
      pthread_join(thread->host_thread, NULL);
      thread->host_thread_valid = false;
    }
    DestroyGuestThread(thread);
  }
  VectorDestruct(&process->threads);
  pthread_key_delete(process->current_thread_key);
  pthread_mutex_destroy(&process->heap_mutex);
  pthread_mutex_destroy(&process->got_resolve_mutex);
  pthread_mutex_destroy(&process->mutex);
  pthread_mutex_destroy(&process->memory_mutex);
  process->initialized = false;
}

ARMGuestThread* ARMProcessAttachMainThread(ARMProcessRuntime* process,
                                           ARMInterpreter* cpu) {
  ARMGuestThread* thread =
      NewGuestThread(process, process->next_tid++, true);
  if (thread == NULL) {
    return NULL;
  }
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
  for (size_t i = 0; i < process->threads.length; i++) {
    ARMGuestThread* thread = process->threads.value.p[i];
    ARMInterpreter* cpu = thread != NULL ? thread->cpu : NULL;
    if (cpu == NULL) {
      continue;
    }
    uint64_t stack_start = cpu->stack_guest_base;
    if (cpu->stack != NULL && addr >= stack_start &&
        addr + size <= stack_start + ARM_STACK_SIZE) {
      void* result = cpu->stack + (addr - stack_start);
      pthread_mutex_unlock(&process->mutex);
      return result;
    }
    uint64_t tls_start = cpu->tls_guest_base;
    if (cpu->tls_block != NULL && addr >= tls_start &&
        addr + size <= tls_start + cpu->tls_block_size) {
      void* result = (char*)cpu->tls_block + (addr - tls_start);
      pthread_mutex_unlock(&process->mutex);
      return result;
    }
  }
  pthread_mutex_unlock(&process->mutex);
  return NULL;
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
  pthread_mutex_lock(&process->mutex);
  if (process->shutting_down) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EINVAL;
  }
  if (process->next_tid > ARM_MAX_GUEST_THREADS) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EAGAIN;
  }
  thread->tid = process->next_tid++;
  pthread_mutex_unlock(&process->mutex);
  if (!InitWorker(thread)) {
    DestroyGuestThread(thread);
    return -ENOMEM;
  }

  pthread_mutex_lock(&process->mutex);
  if (process->shutting_down) {
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -EINVAL;
  }
  VectorAppend(&process->threads, thread);
  int rc = pthread_create(&thread->host_thread, NULL, WorkerEntry, thread);
  if (rc != 0) {
    VectorPop(&process->threads);
    pthread_mutex_unlock(&process->mutex);
    DestroyGuestThread(thread);
    return -ENOMEM;
  }
  thread->host_thread_valid = true;
  pthread_mutex_unlock(&process->mutex);
  return (int32_t)thread->tid;
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
  void* result_host = NULL;
  if (result_ptr != 0) {
    result_host = ARMGuestAddressToHost(caller->cpu, result_ptr, sizeof(int));
    if (result_host == NULL) {
      return -EINVAL;
    }
  }
  ARMProcessRuntime* process = caller->process;
  pthread_mutex_lock(&process->mutex);
  ARMGuestThread* thread = FindThread(process, tid);
  if (thread == NULL) {
    pthread_mutex_unlock(&process->mutex);
    return -ESRCH;
  }
  if (!thread->joinable || !thread->host_thread_valid || thread->is_main) {
    pthread_mutex_unlock(&process->mutex);
    return -EINVAL;
  }
  thread->joinable = false;
  pthread_t host = thread->host_thread;
  pthread_mutex_unlock(&process->mutex);
  pthread_join(host, NULL);
  pthread_mutex_lock(&process->mutex);
  thread->host_thread_valid = false;
  thread->state = kARMGuestThreadJoined;
  int result = thread->exit_code;
  pthread_mutex_unlock(&process->mutex);
  if (result_host != NULL) {
    pthread_mutex_lock(&process->memory_mutex);
    *(int*)result_host = result;
    process->write_epoch++;
    pthread_mutex_unlock(&process->memory_mutex);
  }
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
