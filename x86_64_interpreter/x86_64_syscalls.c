//
//  x86_64_syscalls.c
//  x86_64_interpreter
//

#include "x86_64_syscalls.h"
#include "x86_64_interpreter.h"
#include "x86_64_process.h"
#include "loader_dynamic.h"
#include "loader.h"
#include "loader_lifecycle.h"
#include "elf.h"
#include "x86_64_machine.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static bool InterpreterIsWorker(X86_64Interpreter* interpreter) {
  return interpreter->guest_thread != NULL &&
         !interpreter->guest_thread->is_main;
}

static int64_t InterpreterTerminate(X86_64Interpreter* interpreter,
                                    int64_t status) {
  if (InterpreterIsWorker(interpreter)) {
    X86_64SyscallThreadExit(interpreter->guest_thread, status);
    return 0;
  }
  exit((int)status);
  return -1;
}

static int64_t InterpreterRequestNormalExit(X86_64Interpreter* interpreter,
                                            int64_t status) {
  if (InterpreterIsWorker(interpreter)) {
    X86_64SyscallThreadExit(interpreter->guest_thread, status);
    return 0;
  }
  if (interpreter->loader != NULL) {
    LoaderLifecycleMarkExecutableFiniComplete(interpreter->loader,
                                              interpreter->loader->lifecycle);
  }
  interpreter->exit_code = (int)status;
  interpreter->running = false;
  interpreter->rip = 0;
  return 0;
}

static void LockGotResolve(X86_64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->got_resolve_mutex);
  }
}

static void UnlockGotResolve(X86_64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->got_resolve_mutex);
  }
}

static void ResolveFail(X86_64Interpreter* interpreter, int status) {
  UnlockGotResolve(interpreter);
  X86_64InterpreterFail(interpreter, status);
}

static bool GuestAddressOk(X86_64Interpreter* interpreter, uint64_t addr,
                           size_t size) {
  Loader* loader = interpreter->loader;
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return interpreter->process != NULL &&
         X86_64ProcessGuestMemoryOk(interpreter->process, addr, size);
}

static bool SectionAddressAndSize(LoadedDynamicLibrary* lib, const char* name,
                                  uint64_t* addr, uint64_t* size) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->addr == 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, name) != 0) {
      continue;
    }
    *addr = sh->addr;
    *size = sh->size;
    return true;
  }
  return false;
}

static bool ReadRelaPltEntry(LoadedDynamicLibrary* lib, int64_t index,
                             ELFRelocation* out) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->fd < 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, ".rela.plt") != 0) {
      continue;
    }
    off_t file_offset =
        (off_t)(sh->offset + (uint64_t)index * sizeof(ELFRelocation));
    return pread(lib->fd, out, sizeof(*out), file_offset) ==
           (ssize_t)sizeof(*out);
  }
  return false;
}

static bool GotPointsIntoPlt(Loader* loader, LoadedDynamicLibrary* lib,
                             uint64_t got_value) {
  uint64_t plt_linked = 0;
  uint64_t plt_size = 0;
  if (!SectionAddressAndSize(lib, ".plt", &plt_linked, &plt_size)) {
    return false;
  }
  uint64_t plt_runtime = 0;
  if (!LoaderLinkedAddressToRuntime(loader, lib, plt_linked, &plt_runtime)) {
    return false;
  }
  return got_value >= plt_runtime && got_value < plt_runtime + plt_size;
}

static bool ResolveByPltIndex(X86_64Interpreter* interpreter,
                              LoadedDynamicLibrary* lib, int64_t index,
                              bool* rip_updated) {
  ELFRelocation reloc;
  if (!ReadRelaPltEntry(lib, index, &reloc)) {
    return false;
  }
  if (ELF_R_TYPE(reloc.info) != R_X86_64_JUMP_SLOT) {
    return false;
  }
  int32_t sym_index = ELF_R_SYM(reloc.info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    return false;
  }
  uint64_t symbol_address = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, found_lib, symbol->value,
                                    &symbol_address)) {
    fprintf(stderr, "Cannot translate symbol %s\n", sym_name);
    return false;
  }
  uint64_t got_offset = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, lib, reloc.offset,
                                    &got_offset)) {
    fprintf(stderr, "Cannot translate GOT slot for %s\n", sym_name);
    return false;
  }
  *(uint64_t*)(uintptr_t)got_offset = symbol_address;
  interpreter->rip = symbol_address;
  *rip_updated = true;
  return true;
}

static void ResolveAndFixupSymbol(X86_64Interpreter* interpreter,
                                  bool* rip_updated) {
  LockGotResolve(interpreter);
  LoadedDynamicLibrary* lib =
      (LoadedDynamicLibrary*)interpreter->iregs[X86_REG_RDI];
  int64_t index = (int64_t)interpreter->iregs[X86_REG_RSI];
  if (lib == NULL || index < 0) {
    if (GuestAddressOk(interpreter, interpreter->rsp, 16)) {
      lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->rsp;
      index = *(int64_t*)(uintptr_t)(interpreter->rsp + 8);
    }
  }
  if (lib != NULL && index >= 0 &&
      ResolveByPltIndex(interpreter, lib, index, rip_updated)) {
    UnlockGotResolve(interpreter);
    return;
  }

  lib = (LoadedDynamicLibrary*)interpreter->iregs[X86_REG_RDI];
  if (lib == NULL && GuestAddressOk(interpreter, interpreter->rsp, 8)) {
    lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->rsp;
  }
  if (lib == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    ResolveFail(interpreter, 1);
    return;
  }

  int64_t plt_rel_size = 0;
  const DynamicSection* section = lib->dynamic;
  if (section != NULL) {
    for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
      if (section->entries[i].tag == DT(pltrelsz)) {
        plt_rel_size = section->entries[i].un.val;
      }
    }
  }
  int64_t num_relocations = plt_rel_size / (int64_t)sizeof(ELFRelocation);

  const ELFRelocation* reloc = NULL;
  ELFRelocation file_reloc;
  for (int64_t i = 0; i < num_relocations; i++) {
    if (!ReadRelaPltEntry(lib, i, &file_reloc)) {
      continue;
    }
    if (ELF_R_TYPE(file_reloc.info) != R_X86_64_JUMP_SLOT) {
      continue;
    }
    uint64_t got_slot = 0;
    if (!LoaderLinkedAddressToRuntime(lib->loader, lib, file_reloc.offset,
                                      &got_slot)) {
      continue;
    }
    uint64_t got_value = *(uint64_t*)(uintptr_t)got_slot;
    if (GotPointsIntoPlt(lib->loader, lib, got_value)) {
      reloc = &file_reloc;
      break;
    }
  }
  if (reloc == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    ResolveFail(interpreter, 1);
    return;
  }

  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    ResolveFail(interpreter, 1);
    return;
  }
  uint64_t symbol_address = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, found_lib, symbol->value,
                                    &symbol_address)) {
    fprintf(stderr, "Cannot translate symbol %s\n", sym_name);
    ResolveFail(interpreter, 1);
    return;
  }
  uint64_t got_offset = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, lib, reloc->offset,
                                    &got_offset)) {
    fprintf(stderr, "Cannot translate GOT slot for %s\n", sym_name);
    ResolveFail(interpreter, 1);
    return;
  }
  *(uint64_t*)(uintptr_t)got_offset = symbol_address;
  interpreter->rip = symbol_address;
  *rip_updated = true;
  UnlockGotResolve(interpreter);
}

// The guest libc uses Linux open(2) flag values (see libc/include/fcntl.h),
// but the interpreter services the OPEN syscall through the host's open(),
// whose O_CREAT / O_TRUNC / ... bit values differ (notably on macOS, where
// O_CREAT is 0x0200 rather than Linux's 0100).  Passing the guest bits through
// unchanged silently drops O_CREAT and friends, so a guest fopen(path, "w")
// for a not-yet-existing file fails and returns NULL.  Translate the guest
// (Linux-ABI) flags into the host's flags here.  On a Linux host this is the
// identity mapping; on other hosts it remaps each bit.
static int TranslateGuestOpenFlags(int guest_flags) {
  enum {
    kGuestO_ACCMODE   = 00000003,
    kGuestO_CREAT     = 00000100,
    kGuestO_EXCL      = 00000200,
    kGuestO_NOCTTY    = 00000400,
    kGuestO_TRUNC     = 00001000,
    kGuestO_APPEND    = 00002000,
    kGuestO_NONBLOCK  = 00004000,
    kGuestO_SYNC      = 00010000,
    kGuestO_DIRECTORY = 00200000,
    kGuestO_NOFOLLOW  = 00400000,
    kGuestO_CLOEXEC   = 02000000,
  };
  // Access mode (O_RDONLY / O_WRONLY / O_RDWR) shares 0/1/2 with the host.
  int host = guest_flags & kGuestO_ACCMODE;
  if (guest_flags & kGuestO_CREAT)     host |= O_CREAT;
  if (guest_flags & kGuestO_EXCL)      host |= O_EXCL;
  if (guest_flags & kGuestO_NOCTTY)    host |= O_NOCTTY;
  if (guest_flags & kGuestO_TRUNC)     host |= O_TRUNC;
  if (guest_flags & kGuestO_APPEND)    host |= O_APPEND;
  if (guest_flags & kGuestO_NONBLOCK)  host |= O_NONBLOCK;
  if (guest_flags & kGuestO_SYNC)      host |= O_SYNC;
  if (guest_flags & kGuestO_DIRECTORY) host |= O_DIRECTORY;
  if (guest_flags & kGuestO_NOFOLLOW)  host |= O_NOFOLLOW;
#ifdef O_CLOEXEC
  if (guest_flags & kGuestO_CLOEXEC)   host |= O_CLOEXEC;
#endif
  return host;
}

int64_t X86_64HandleSyscall(X86_64Interpreter* interpreter, int64_t number,
                            int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                            int64_t a4, int64_t a5) {
  (void)a4;
  (void)a5;
  switch (number) {
    case X86_64_SYSCALL_HALT:
    case X86_64_SYSCALL_EXIT:
      return InterpreterTerminate(interpreter, a0);
    case X86_64_SYSCALL_EXIT_CLEAN:
      return InterpreterRequestNormalExit(interpreter, a0);
    case X86_64_SYSCALL_OPEN:
      return open((const char*)(uintptr_t)a0,
                  TranslateGuestOpenFlags((int)a1), (mode_t)a2);
    case X86_64_SYSCALL_CLOSE:
      return close((int)a0);
    case X86_64_SYSCALL_READ:
      return read((int)a0, (void*)(uintptr_t)a1, (size_t)a2);
    case X86_64_SYSCALL_WRITE:
      return write((int)a0, (const void*)(uintptr_t)a1, (size_t)a2);
    case X86_64_SYSCALL_LSEEK:
      return lseek((int)a0, (off_t)a1, (int)a2);
    case X86_64_SYSCALL_MALLOC:
      return (int64_t)(uintptr_t)malloc((size_t)a0);
    case X86_64_SYSCALL_REALLOC:
      return (int64_t)(uintptr_t)realloc((void*)(uintptr_t)a0, (size_t)a1);
    case X86_64_SYSCALL_FREE:
      free((void*)(uintptr_t)a0);
      return 0;
    case X86_64_SYSCALL_ABORT:
      abort();
      break;
    case X86_64_SYSCALL_TIME:
      return (int64_t)time(NULL);
    case X86_64_SYSCALL_CLOCK: {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
      }
      return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
    }
    case X86_64_SYSCALL_RESOLVE: {
      bool rip_updated = false;
      ResolveAndFixupSymbol(interpreter, &rip_updated);
      interpreter->rip_updated = rip_updated;
      return 0;
    }
    case X86_64_SYSCALL_THREAD_CREATE:
      return X86_64SyscallThreadCreate(interpreter->guest_thread, (uint64_t)a0,
                                       (uint64_t)a1, (uint64_t)a2,
                                       (uint64_t)a3);
    case X86_64_SYSCALL_THREAD_JOIN:
      return X86_64SyscallThreadJoin(interpreter->guest_thread, (uint64_t)a0,
                                     (uint64_t)a1);
    case X86_64_SYSCALL_THREAD_SELF:
      return X86_64SyscallThreadSelf(interpreter->guest_thread);
    case X86_64_SYSCALL_GET_TP:
      return X86_64SyscallGetTp(interpreter->guest_thread);
    case X86_64_SYSCALL_THREAD_EXIT:
      InterpreterTerminate(interpreter, a0);
      return 0;
    case X86_64_SYSCALL_HEAP_LOCK:
      return X86_64SyscallHeapLock(interpreter->guest_thread);
    case X86_64_SYSCALL_HEAP_UNLOCK:
      return X86_64SyscallHeapUnlock(interpreter->guest_thread);
    case X86_64_SYSCALL_THREAD_YIELD:
      return sched_yield();
    case X86_64_SYSCALL_MONOTONIC_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)(uintptr_t)a0;
      if (result == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    case X86_64_SYSCALL_THREAD_DETACH:
      return X86_64SyscallThreadDetach(interpreter->guest_thread, (uint64_t)a0);
    case X86_64_SYSCALL_ADDR_WAIT:
      return X86_64SyscallAddrWait(interpreter->guest_thread, (uint64_t)a0,
                                   (uint64_t)a1, (size_t)a2, a3);
    case X86_64_SYSCALL_ADDR_WAKE:
      return X86_64SyscallAddrWake(interpreter->guest_thread, (uint64_t)a0,
                                   a1 != 0);
    case X86_64_SYSCALL_THREAD_SLEEP:
      return X86_64SyscallThreadSleep(interpreter->guest_thread, (uint64_t)a0,
                                      (uint64_t)a1);
    case X86_64_SYSCALL_HARDWARE_CONCURRENCY:
      return X86_64SyscallHardwareConcurrency();
    case X86_64_SYSCALL_REALTIME_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)(uintptr_t)a0;
      if (result == NULL || clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    default:
      fprintf(stderr, "Unknown x86_64 syscall %lld\n", (long long)number);
      X86_64InterpreterFail(interpreter, 1);
  }
  return -1;
}
