//
//  aarch64_syscalls.c
//  aarch64_interpreter
//

#include "aarch64_syscalls.h"
#include "aarch64_interpreter.h"
#include "aarch64_process.h"
#include "loader_dynamic.h"
#include "loader.h"
#include "loader_lifecycle.h"
#include "elf.h"
#include "chrono_host.h"
#include "filesystem_host.h"
#include "random_host.h"
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

static bool InterpreterIsWorker(AARCH64Interpreter* interpreter) {
  return interpreter->guest_thread != NULL &&
         !interpreter->guest_thread->is_main;
}

static int64_t InterpreterTerminate(AARCH64Interpreter* interpreter,
                                    int64_t status) {
  if (InterpreterIsWorker(interpreter)) {
    AARCH64SyscallThreadExit(interpreter->guest_thread, status);
    return 0;
  }
  exit((int)status);
  return -1;
}

static int64_t InterpreterRequestNormalExit(AARCH64Interpreter* interpreter,
                                            int64_t status) {
  if (InterpreterIsWorker(interpreter)) {
    AARCH64SyscallThreadExit(interpreter->guest_thread, status);
    return 0;
  }
  if (interpreter->loader != NULL) {
    LoaderLifecycleMarkExecutableFiniComplete(interpreter->loader,
                                              interpreter->loader->lifecycle);
  }
  interpreter->exit_code = (int)status;
  interpreter->running = false;
  interpreter->pc = 0;
  return 0;
}

static void LockGotResolve(AARCH64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->got_resolve_mutex);
  }
}

static void UnlockGotResolve(AARCH64Interpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->got_resolve_mutex);
  }
}

static void ResolveFail(AARCH64Interpreter* interpreter, int status) {
  UnlockGotResolve(interpreter);
  AARCH64InterpreterFail(interpreter, status);
}

static bool GuestAddressOk(Loader* loader, uint64_t addr, size_t size) {
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

static bool ResolveByPltIndex(AARCH64Interpreter* interpreter,
                              LoadedDynamicLibrary* lib, int64_t index,
                              bool* pc_updated) {
  ELFRelocation reloc;
  if (!ReadRelaPltEntry(lib, index, &reloc)) {
    return false;
  }
  if (ELF_R_TYPE(reloc.info) != R_AARCH64_JUMP_SLOT) {
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
  interpreter->pc = symbol_address - 4;
  *pc_updated = true;
  return true;
}

static void ResolveAndFixupSymbol(AARCH64Interpreter* interpreter,
                                  bool* pc_updated) {
  LockGotResolve(interpreter);
  LoadedDynamicLibrary* lib = (LoadedDynamicLibrary*)interpreter->x[0];
  int64_t index = (int64_t)interpreter->x[1];
  if (lib == NULL || index < 0) {
    if (GuestAddressOk(interpreter->loader, interpreter->sp, 16)) {
      lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->sp;
      index = *(int64_t*)(uintptr_t)(interpreter->sp + 8);
    }
  }
  if (lib != NULL && index >= 0 &&
      ResolveByPltIndex(interpreter, lib, index, pc_updated)) {
    UnlockGotResolve(interpreter);
    return;
  }

  lib = (LoadedDynamicLibrary*)interpreter->x[0];
  if (lib == NULL && GuestAddressOk(interpreter->loader, interpreter->sp, 8)) {
    lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->sp;
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
    if (ELF_R_TYPE(file_reloc.info) != R_AARCH64_JUMP_SLOT) {
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
  interpreter->pc = symbol_address - 4;
  *pc_updated = true;
  UnlockGotResolve(interpreter);
}

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

int64_t AARCH64HandleSyscall(AARCH64Interpreter* interpreter, int64_t number,
                             int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                             int64_t a4, int64_t a5) {
  (void)a4;
  (void)a5;
  switch (number) {
    case AARCH64_SYSCALL_HALT:
    case AARCH64_SYSCALL_EXIT:
      return InterpreterTerminate(interpreter, a0);
    case AARCH64_SYSCALL_EXIT_CLEAN:
      return InterpreterRequestNormalExit(interpreter, a0);
    case AARCH64_SYSCALL_OPEN:
      return open((const char*)(uintptr_t)a0,
                  TranslateGuestOpenFlags((int)a1), (mode_t)a2);
    case AARCH64_SYSCALL_CLOSE:
      return close((int)a0);
    case AARCH64_SYSCALL_READ:
      return read((int)a0, (void*)(uintptr_t)a1, (size_t)a2);
    case AARCH64_SYSCALL_WRITE:
      return write((int)a0, (const void*)(uintptr_t)a1, (size_t)a2);
    case AARCH64_SYSCALL_LSEEK:
      return lseek((int)a0, (off_t)a1, (int)a2);
    case AARCH64_SYSCALL_MALLOC:
      return (int64_t)(uintptr_t)malloc((size_t)a0);
    case AARCH64_SYSCALL_REALLOC:
      return (int64_t)(uintptr_t)realloc((void*)(uintptr_t)a0, (size_t)a1);
    case AARCH64_SYSCALL_FREE:
      free((void*)(uintptr_t)a0);
      return 0;
    case AARCH64_SYSCALL_ABORT:
      abort();
      break;
    case AARCH64_SYSCALL_TIME:
      return (int64_t)time(NULL);
    case AARCH64_SYSCALL_CLOCK: {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
      }
      return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
    }
    case AARCH64_SYSCALL_RESOLVE: {
      bool pc_updated = false;
      ResolveAndFixupSymbol(interpreter, &pc_updated);
      (void)pc_updated;
      return 0;
    }
    case AARCH64_SYSCALL_THREAD_CREATE:
      return AARCH64SyscallThreadCreate(interpreter->guest_thread, (uint64_t)a0,
                                        (uint64_t)a1, (uint64_t)a2,
                                        (uint64_t)a3);
    case AARCH64_SYSCALL_THREAD_JOIN:
      return AARCH64SyscallThreadJoin(interpreter->guest_thread, (uint64_t)a0,
                                      (uint64_t)a1);
    case AARCH64_SYSCALL_THREAD_SELF:
      return AARCH64SyscallThreadSelf(interpreter->guest_thread);
    case AARCH64_SYSCALL_GET_TP:
      return AARCH64SyscallGetTp(interpreter->guest_thread);
    case AARCH64_SYSCALL_THREAD_EXIT:
      InterpreterTerminate(interpreter, a0);
      return 0;
    case AARCH64_SYSCALL_HEAP_LOCK:
      return AARCH64SyscallHeapLock(interpreter->guest_thread);
    case AARCH64_SYSCALL_HEAP_UNLOCK:
      return AARCH64SyscallHeapUnlock(interpreter->guest_thread);
    case AARCH64_SYSCALL_THREAD_YIELD:
      return sched_yield();
    case AARCH64_SYSCALL_MONOTONIC_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)(uintptr_t)a0;
      if (result == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    case AARCH64_SYSCALL_THREAD_DETACH:
      return AARCH64SyscallThreadDetach(interpreter->guest_thread, (uint64_t)a0);
    case AARCH64_SYSCALL_ADDR_WAIT:
      return AARCH64SyscallAddrWait(interpreter->guest_thread, (uint64_t)a0,
                                    (uint64_t)a1, (size_t)a2, a3);
    case AARCH64_SYSCALL_ADDR_WAKE:
      return AARCH64SyscallAddrWake(interpreter->guest_thread, (uint64_t)a0,
                                    a1 != 0);
    case AARCH64_SYSCALL_THREAD_SLEEP:
      return AARCH64SyscallThreadSleep(interpreter->guest_thread, (uint64_t)a0,
                                       (uint64_t)a1);
    case AARCH64_SYSCALL_HARDWARE_CONCURRENCY:
      return AARCH64SyscallHardwareConcurrency();
    case AARCH64_SYSCALL_REALTIME_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)(uintptr_t)a0;
      if (result == NULL || clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return -1;
      }
      *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
      return 0;
    }
    case AARCH64_SYSCALL_FS_STATUS:
      return DaveHostFilesystemGetStatus(
          (const char*)(uintptr_t)a0, (int)a1,
          (DaveHostFilesystemStat*)(uintptr_t)a2);
    case AARCH64_SYSCALL_FS_OPEN_DIRECTORY:
      return DaveHostFilesystemOpenDirectory((const char*)(uintptr_t)a0);
    case AARCH64_SYSCALL_FS_READ_DIRECTORY:
      return DaveHostFilesystemReadDirectory(
          (int)a0, (DaveHostFilesystemDirectoryEntry*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_CLOSE_DIRECTORY:
      return DaveHostFilesystemCloseDirectory((int)a0);
    case AARCH64_SYSCALL_FS_CREATE_DIRECTORY:
      return DaveHostFilesystemCreateDirectory((const char*)(uintptr_t)a0,
                                               (uint32_t)a1);
    case AARCH64_SYSCALL_FS_REMOVE:
      return DaveHostFilesystemRemove((const char*)(uintptr_t)a0);
    case AARCH64_SYSCALL_FS_RENAME:
      return DaveHostFilesystemRename((const char*)(uintptr_t)a0,
                                      (const char*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_CURRENT_PATH:
      return DaveHostFilesystemCurrentPath((char*)(uintptr_t)a0, (size_t)a1);
    case AARCH64_SYSCALL_FS_SET_CURRENT_PATH:
      return DaveHostFilesystemSetCurrentPath((const char*)(uintptr_t)a0);
    case AARCH64_SYSCALL_FS_READ_SYMLINK:
      return DaveHostFilesystemReadSymlink(
          (const char*)(uintptr_t)a0, (char*)(uintptr_t)a1, (size_t)a2);
    case AARCH64_SYSCALL_FS_CREATE_SYMLINK:
      return DaveHostFilesystemCreateSymlink((const char*)(uintptr_t)a0,
                                             (const char*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_CREATE_HARD_LINK:
      return DaveHostFilesystemCreateHardLink((const char*)(uintptr_t)a0,
                                              (const char*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_SET_PERMISSIONS:
      return DaveHostFilesystemSetPermissions(
          (const char*)(uintptr_t)a0, (uint32_t)a1, (int)a2);
    case AARCH64_SYSCALL_FS_RESIZE:
      return a1 == 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostFilesystemResize(
                       (const char*)(uintptr_t)a0,
                       *(const uint64_t*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_SET_MODIFICATION_TIME:
      return a1 == 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostFilesystemSetModificationTime(
                       (const char*)(uintptr_t)a0,
                       *(const int64_t*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_SPACE:
      return DaveHostFilesystemQuerySpace(
          (const char*)(uintptr_t)a0,
          (DaveHostFilesystemSpace*)(uintptr_t)a1);
    case AARCH64_SYSCALL_FS_COPY_FILE:
      return DaveHostFilesystemCopyFile((const char*)(uintptr_t)a0,
                                        (const char*)(uintptr_t)a1, (int)a2);
    case AARCH64_SYSCALL_FS_CANONICAL:
      return DaveHostFilesystemCanonical(
          (const char*)(uintptr_t)a0, (char*)(uintptr_t)a1, (size_t)a2);
    case AARCH64_SYSCALL_TZDB_VERSION:
      return DaveHostChronoTzdbVersion((char*)(uintptr_t)a0, (size_t)a1);
    case AARCH64_SYSCALL_TZDB_GENERATION:
      return a0 == 0 ? -DAVE_HOST_EINVAL
                     : DaveHostChronoGeneration((uint64_t*)(uintptr_t)a0);
    case AARCH64_SYSCALL_TZDB_RELOAD:
      return DaveHostChronoReload((uint64_t*)(uintptr_t)a0);
    case AARCH64_SYSCALL_TZDB_CURRENT_ZONE:
      return DaveHostChronoCurrentZone((char*)(uintptr_t)a0, (size_t)a1);
    case AARCH64_SYSCALL_TZDB_ZONE_COUNT:
      return a0 == 0 ? -DAVE_HOST_EINVAL
                     : DaveHostChronoZoneCount((uint32_t*)(uintptr_t)a0);
    case AARCH64_SYSCALL_TZDB_ZONE_NAME:
      return DaveHostChronoZoneName((uint32_t)a0, (char*)(uintptr_t)a1,
                                    (size_t)a2);
    case AARCH64_SYSCALL_TZDB_LOCATE_ZONE:
      return a2 == 0 || a3 == 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostChronoLocateZone(
                       (const char*)(uintptr_t)a0, (char*)(uintptr_t)a1,
                       (size_t)a2, (uint32_t*)(uintptr_t)a3);
    case AARCH64_SYSCALL_TZDB_SYS_INFO: {
      const DaveHostChronoSysInfoRequestWire* request =
          (const DaveHostChronoSysInfoRequestWire*)(uintptr_t)a1;
      return a0 == 0 || request == NULL
                 ? -DAVE_HOST_EINVAL
                 : DaveHostChronoSysInfoRequest((const char*)(uintptr_t)a0,
                                                  request);
    }
    case AARCH64_SYSCALL_TZDB_LOCAL_INFO: {
      const DaveHostChronoLocalInfoRequestWire* request =
          (const DaveHostChronoLocalInfoRequestWire*)(uintptr_t)a1;
      return a0 == 0 || request == NULL
                 ? -DAVE_HOST_EINVAL
                 : DaveHostChronoLocalInfoRequest((const char*)(uintptr_t)a0,
                                                    request);
    }
    case AARCH64_SYSCALL_TZDB_LEAP_COUNT:
      return a0 == 0 ? -DAVE_HOST_EINVAL
                     : DaveHostChronoLeapCount((uint32_t*)(uintptr_t)a0);
    case AARCH64_SYSCALL_TZDB_LEAP_INFO:
      return a1 == 0 ? -DAVE_HOST_EINVAL
                     : DaveHostChronoLeapInfo(
                           (uint32_t)a0,
                           (DaveHostChronoLeapSecond*)(uintptr_t)a1);
    case AARCH64_SYSCALL_RANDOM_BYTES:
      return a0 == 0 && a1 != 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostRandomBytes((void*)(uintptr_t)a0, (size_t)a1);
    default:
      fprintf(stderr, "Unknown AArch64 syscall %lld\n", (long long)number);
      AARCH64InterpreterFail(interpreter, 1);
  }
  return -1;
}
