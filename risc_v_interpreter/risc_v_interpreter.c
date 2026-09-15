//
//  risc_v_interpreter.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_interpreter.h"
#include <fcntl.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include "elf.h"
#include "loader_lifecycle.h"
#include "chrono_host.h"
#include "filesystem_host.h"
#include "random_host.h"
#include <errno.h>
#include "risc_v_disassembler.h"
#include "risc_v_process.h"

static void GuestMemoryLock(RISCVInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->memory_mutex);
  }
}

static void GuestMemoryUnlock(RISCVInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->memory_mutex);
  }
}

static uint64_t GuestAddr32(int64_t addr) { return (uint32_t)addr; }

static int ShiftMask(const RISCVInterpreter* interpreter) {
  return interpreter->xlen32 ? 31 : 63;
}

static uint64_t LogicalBits(const RISCVInterpreter* interpreter, int64_t v) {
  return interpreter->xlen32 ? (uint32_t)v : (uint64_t)v;
}

static uint64_t GuestCallPC(RISCVInterpreter* interpreter, uint64_t fn) {
  if (!interpreter->xlen32 || interpreter->loader == NULL) {
    return fn;
  }
  uint64_t linked = fn;
  if (LoaderRuntimeAddressToLinked(interpreter->loader, fn, &linked)) {
    return linked;
  }
  return GuestAddr32((int64_t)fn);
}

static uint64_t GuestCallReturnPC(const RISCVInterpreter* interpreter) {
  if (interpreter->xlen32) {
    return interpreter->stack_guest_base + sizeof(interpreter->startup_code);
  }
  return (uint64_t)(uintptr_t)interpreter->call_return_code;
}

static bool AddressInExecutableRegion(Loader* loader, uint64_t addr) {
  if (loader == NULL || addr == 0) {
    return false;
  }
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

static void* GuestMem(RISCVInterpreter* interpreter, int64_t addr, size_t size) {
  uint64_t a = interpreter->xlen32 ? GuestAddr32(addr) : (uint64_t)addr;
  if (interpreter->stack != NULL) {
    uint64_t start = interpreter->stack_guest_base;
    uint64_t end = start + RISC_V_STACK_SIZE;
    if (a >= start && a + size <= end) {
      return interpreter->stack + (a - start);
    }
  }
  if (interpreter->tls_block != NULL && interpreter->tls_block_size > 0) {
    uint64_t start = interpreter->tls_guest_base;
    uint64_t end = start + interpreter->tls_block_size;
    if (a >= start && a + size <= end) {
      return (char*)interpreter->tls_block + (a - start);
    }
  }
  if (interpreter->xlen32 && interpreter->process != NULL) {
    void* mapped =
        RISCVProcessResolveGuestAddress(interpreter->process, a, size);
    if (mapped != NULL) {
      return mapped;
    }
  }
  if (interpreter->xlen32 && interpreter->loader != NULL) {
    uint64_t host = 0;
    if (LoaderLinkedAddressToRuntime(interpreter->loader, NULL, a, &host)) {
      return (void*)(uintptr_t)host;
    }
    return NULL;
  }
  return (void*)(uintptr_t)a;
}

static void* RequireGuestMem(RISCVInterpreter* interpreter, int64_t addr,
                             size_t size) {
  void* p = GuestMem(interpreter, addr, size);
  if (p == NULL) {
    fprintf(stderr, "Invalid RISC-V guest address 0x%" PRIx64 "\n",
            interpreter->xlen32 ? GuestAddr32(addr) : (uint64_t)addr);
    RISCVInterpreterDumpRegisters(interpreter);
    exit(1);
  }
  return p;
}

static uint32_t FetchInst(RISCVInterpreter* interpreter) {
  return *(uint32_t*)RequireGuestMem(interpreter, interpreter->pc, 4);
}

static void GuestMemoryDidWrite(RISCVInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    interpreter->process->write_epoch++;
  }
  interpreter->reservation_valid = false;
}

void RISCVInterpreterDumpRegisters(RISCVInterpreter* interpreter) {
  const int kWidth = 40;
  for (int i = 0; i < RV_NUM_INT_REGS; i += 2) {
    DisassemblePrintRegister(stdout, i, kRegTypeInt, "");

    int n = printf("  0x%016" PRIx64 " (%" PRId64 ")", interpreter->iregs[i],
           interpreter->iregs[i]);
    n = kWidth - n;
    while (n-- > 0) {
      putchar(' ');
    }
    DisassemblePrintRegister(stdout, i+1, kRegTypeInt, "");
    printf("  0x%016" PRIx64 " (%" PRId64 ")", interpreter->iregs[i+1],
           interpreter->iregs[i+1]);
    printf("\n");
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i += 2) {
    DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
    int n =  printf("  %g  ", interpreter->fregs[i]);
    n = kWidth - n;
    while (n-- > 0) {
      putchar(' ');
    }
    DisassemblePrintRegister(stdout, i+1, kRegTypeFloat, "");
    printf("  %g", interpreter->fregs[i+1]);
    printf("\n");
  }
  printf("pc: 0x%" PRIx64 " (%" PRId64 ")\n", interpreter->pc, interpreter->pc);
}

static void DumpStateAndExit(RISCVInterpreter* interpreter) {
  RISCVInterpreterDumpRegisters(interpreter);
  exit(1);
}

// On entry:
// t0: address of resolver data structure.  This is called "link_map" in
//     Linux, but in this loader it's the address of the LoadedDynamicLibrary
//     that contains the GOT entry being resolved.
// t1: the byte offset from the start of the GOTPLT (the part of the GOT
//     that contains addresses of functions rather than data) to the GOT
//     entry that is being resolved.
//
// The procedure is to use the offset into the GOTPLT to find a relocation
// for the GOT entry.  This gives us an ELFSymbol, from which we get the
// symbol name.  This is looked up in the dynamic symbol tables and if
// found, the GOT entry is set to the address of the symbol and the PC
// is set to that address.
static void ResolveAndFixupSymbol(RISCVInterpreter* interpreter) {
  if (interpreter->process != NULL) {
    pthread_mutex_lock(&interpreter->process->got_resolve_mutex);
  }
  const int t0 = 5;
  const int t1 = 6;
  int64_t offset = interpreter->iregs[t1];
  LoadedDynamicLibrary* lib = (LoadedDynamicLibrary*)interpreter->iregs[t0];
  const void* relocs =
      DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(jmprel));
  if (relocs == NULL) {
    fprintf(stderr, "Failed to find relocations in library %s\n",
            lib->filename.value);
    exit(1);
  }
  // The offset is a byte offset into the .got.plt.  We need to find the
  // relocation which will be at index offset/8 into the relocation table.
  // TODO: is it safe to assume that relocations are in order in the
  // .rela.plt table or do we need to search for the offset?
  int64_t reloc_index = offset / 8;

  const ELFRelocation* reloc = (const ELFRelocation*)relocs + reloc_index;
  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  // printf("Resolving symbol %s\n", sym_name);
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t symbol_address = found_lib->load_address + symbol->value;

  // Fixup GOT entry to contain the symbol address.
  GuestMemoryLock(interpreter);
  *(uint64_t*)(lib->load_address + reloc->offset) = symbol_address;
  GuestMemoryDidWrite(interpreter);
  GuestMemoryUnlock(interpreter);
  if (interpreter->process != NULL) {
    pthread_mutex_unlock(&interpreter->process->got_resolve_mutex);
  }

  // Finally jump to the address.  The PC will be incremented after the
  // ecall instruction so we need to set it to one instruction before
  // the address we want - 4 bytes.
  interpreter->pc = symbol_address - 4;
}

const bool kDumpRegsonEbreak = false;
const bool kShowRegChanges = false;

static int TranslateGuestOpenFlags(int guest_flags) {
  enum {
    kGuestAccmode = 00000003,
    kGuestCreate = 00000100,
    kGuestExclusive = 00000200,
    kGuestNoTty = 00000400,
    kGuestTruncate = 00001000,
    kGuestAppend = 00002000,
    kGuestNonblock = 00004000,
    kGuestSync = 00010000,
    kGuestDirectory = 00200000,
    kGuestNoFollow = 00400000,
  };
  int host = guest_flags & kGuestAccmode;
  if (guest_flags & kGuestCreate) host |= O_CREAT;
  if (guest_flags & kGuestExclusive) host |= O_EXCL;
  if (guest_flags & kGuestNoTty) host |= O_NOCTTY;
  if (guest_flags & kGuestTruncate) host |= O_TRUNC;
  if (guest_flags & kGuestAppend) host |= O_APPEND;
  if (guest_flags & kGuestNonblock) host |= O_NONBLOCK;
  if (guest_flags & kGuestSync) host |= O_SYNC;
  if (guest_flags & kGuestDirectory) host |= O_DIRECTORY;
  if (guest_flags & kGuestNoFollow) host |= O_NOFOLLOW;
  return host;
}

static void* GuestMem(RISCVInterpreter* interpreter, int64_t addr, size_t size);

static void* EcallGuestPtr(RISCVInterpreter* interpreter, int64_t addr,
                           size_t size) {
  if (!interpreter->xlen32) {
    return (void*)(uintptr_t)addr;
  }
  return GuestMem(interpreter, addr, size);
}

static void HandleEcall(RISCVInterpreter* interpreter) {
  switch (interpreter->iregs[REG(t6)]) {
    case RISC_V_ECALL_HALT:
      interpreter->exit_code = (int)interpreter->iregs[REG(a0)];
      interpreter->pc = 0;
      break;
    case RISC_V_ECALL_EXIT:
      if (interpreter->guest_thread != NULL &&
          !interpreter->guest_thread->is_main) {
        RISCVSyscallThreadExit(interpreter->guest_thread,
                               interpreter->iregs[REG(a1)]);
      } else {
        exit((int)interpreter->iregs[REG(a1)]);
      }
      break;
    case RISC_V_ECALL_EXIT_CLEAN:
      if (interpreter->loader != NULL) {
        LoaderLifecycleMarkExecutableFiniComplete(
            interpreter->loader, interpreter->loader->lifecycle);
      }
      interpreter->exit_code = (int)interpreter->iregs[REG(a1)];
      interpreter->pc = 0;
      break;
    case RISC_V_ECALL_NESTED_RETURN:
      interpreter->pc = 0;
      break;
    case RISC_V_ECALL_OPEN: {
      const char* filename = (const char*)EcallGuestPtr(
          interpreter, interpreter->iregs[REG(a1)], 1);
      int flags =
          TranslateGuestOpenFlags((int)interpreter->iregs[REG(a2)]);
      mode_t create_mode = (mode_t)interpreter->iregs[REG(a3)];
      interpreter->iregs[REG(a0)] = open(filename, flags, create_mode);
      break;
    }
    case RISC_V_ECALL_CLOSE: {
      int fd = (int)interpreter->iregs[REG(a1)];
      interpreter->iregs[REG(a0)] = close(fd);
      break;
    }
    case RISC_V_ECALL_READ: {
      int fd = (int)interpreter->iregs[REG(a1)];
      size_t size = (size_t)interpreter->iregs[REG(a3)];
      void* addr = EcallGuestPtr(interpreter, interpreter->iregs[REG(a2)], size);
      interpreter->iregs[REG(a0)] = read(fd, addr, size);
      break;
    }
    case RISC_V_ECALL_WRITE: {
      int fd = (int)interpreter->iregs[REG(a1)];
      size_t size = (size_t)interpreter->iregs[REG(a3)];
      const void* addr =
          EcallGuestPtr(interpreter, interpreter->iregs[REG(a2)], size);
      interpreter->iregs[REG(a0)] = write(fd, addr, size);
      break;
    }
    case RISC_V_ECALL_LSEEK: {
      int fd = (int)interpreter->iregs[REG(a1)];
      off_t pos = (off_t)interpreter->iregs[REG(a2)];
      int whence = (int)interpreter->iregs[REG(a3)];
      interpreter->iregs[REG(a0)] = lseek(fd, pos, whence);
      break;
    }

    case RISC_V_ECALL_RESOLVE: {
      ResolveAndFixupSymbol(interpreter);
      break;
    }
    case RISC_V_ECALL_MALLOC: {
      size_t size = (size_t)interpreter->iregs[REG(a1)];
      if (interpreter->xlen32) {
        uint32_t* addr = EcallGuestPtr(
            interpreter, interpreter->iregs[REG(a2)], sizeof(uint32_t));
        uint32_t guest_address = 0;
        bool ok = addr != NULL && RISCVProcessGuestHeapMalloc(
                                      interpreter->process, size,
                                      &guest_address);
        if (addr != NULL) {
          *addr = guest_address;
        }
        interpreter->iregs[REG(a0)] = ok;
        break;
      }
      void** addr = (void**)EcallGuestPtr(interpreter, interpreter->iregs[REG(a2)],
                                          sizeof(void*));
      if (addr == NULL) {
        interpreter->iregs[REG(a0)] = 0;
        break;
      }
      *addr = malloc(size);
      interpreter->iregs[REG(a0)] = *addr != NULL;
      break;
    }
    case RISC_V_ECALL_REALLOC: {
      if (interpreter->xlen32) {
        uint32_t old_guest_address =
            (uint32_t)interpreter->iregs[REG(a1)];
        size_t size = (size_t)interpreter->iregs[REG(a2)];
        uint32_t* addr = EcallGuestPtr(
            interpreter, interpreter->iregs[REG(a3)], sizeof(uint32_t));
        uint32_t new_guest_address = 0;
        bool ok = addr != NULL && RISCVProcessGuestHeapRealloc(
                                      interpreter->process,
                                      old_guest_address, size,
                                      &new_guest_address);
        if (addr != NULL) {
          *addr = new_guest_address;
        }
        interpreter->iregs[REG(a0)] = ok;
        break;
      }
      void* old_addr =
          EcallGuestPtr(interpreter, interpreter->iregs[REG(a1)], 1);
      size_t size = (size_t)interpreter->iregs[REG(a2)];
      void** addr = (void**)EcallGuestPtr(interpreter, interpreter->iregs[REG(a3)],
                                          sizeof(void*));
      if (addr == NULL) {
        interpreter->iregs[REG(a0)] = 0;
        break;
      }
      *addr = realloc(old_addr, size);
      interpreter->iregs[REG(a0)] = *addr != NULL;
      break;
    }
    case RISC_V_ECALL_FREE: {
      if (interpreter->xlen32) {
        RISCVProcessGuestHeapFree(
            interpreter->process, (uint32_t)interpreter->iregs[REG(a1)]);
        break;
      }
      void* addr = EcallGuestPtr(interpreter, interpreter->iregs[REG(a1)], 1);
      free(addr);
      break;
    }
    case RISC_V_ECALL_ABORT: {
      abort();
      break;
    }
    case RISC_V_ECALL_TIME:
      interpreter->iregs[REG(a0)] = RISCVSyscallTime();
      break;
    case RISC_V_ECALL_CLOCK:
      interpreter->iregs[REG(a0)] = RISCVSyscallClock();
      break;
    case RISC_V_ECALL_THREAD_CREATE:
      interpreter->iregs[REG(a0)] = RISCVSyscallThreadCreate(
          interpreter->guest_thread, interpreter->iregs[REG(a1)],
          interpreter->iregs[REG(a2)], interpreter->iregs[REG(a3)],
          interpreter->iregs[REG(a4)]);
      break;
    case RISC_V_ECALL_THREAD_JOIN:
      interpreter->iregs[REG(a0)] = RISCVSyscallThreadJoin(
          interpreter->guest_thread, interpreter->iregs[REG(a1)],
          interpreter->iregs[REG(a2)]);
      break;
    case RISC_V_ECALL_THREAD_SELF:
      interpreter->iregs[REG(a0)] =
          RISCVSyscallThreadSelf(interpreter->guest_thread);
      break;
    case RISC_V_ECALL_GET_TP:
      interpreter->iregs[REG(a0)] =
          RISCVSyscallGetTp(interpreter->guest_thread);
      break;
    case RISC_V_ECALL_THREAD_EXIT:
      RISCVSyscallThreadExit(interpreter->guest_thread,
                             interpreter->iregs[REG(a1)]);
      break;
    case RISC_V_ECALL_HEAP_LOCK:
      interpreter->iregs[REG(a0)] =
          RISCVSyscallHeapLock(interpreter->guest_thread);
      break;
    case RISC_V_ECALL_HEAP_UNLOCK:
      interpreter->iregs[REG(a0)] =
          RISCVSyscallHeapUnlock(interpreter->guest_thread);
      break;
    case RISC_V_ECALL_THREAD_YIELD:
      interpreter->iregs[REG(a0)] = sched_yield();
      break;
    case RISC_V_ECALL_MONOTONIC_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)RISCVGuestAddressToHost(
          interpreter, (uint64_t)interpreter->iregs[REG(a1)], sizeof(int64_t));
      if (result == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        interpreter->iregs[REG(a0)] = (uint64_t)-1;
      } else {
        *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
        interpreter->iregs[REG(a0)] = 0;
      }
      break;
    }
    case RISC_V_ECALL_THREAD_DETACH:
      interpreter->iregs[REG(a0)] = (uint64_t)RISCVSyscallThreadDetach(
          interpreter->guest_thread, (uint64_t)interpreter->iregs[REG(a1)]);
      break;
    case RISC_V_ECALL_ADDR_WAIT:
      interpreter->iregs[REG(a0)] = (uint64_t)RISCVSyscallAddrWait(
          interpreter->guest_thread, (uint64_t)interpreter->iregs[REG(a1)],
          (uint64_t)interpreter->iregs[REG(a2)],
          (uint64_t)interpreter->iregs[REG(a3)],
          (int64_t)interpreter->iregs[REG(a4)]);
      break;
    case RISC_V_ECALL_ADDR_WAKE:
      interpreter->iregs[REG(a0)] = (uint64_t)RISCVSyscallAddrWake(
          interpreter->guest_thread, (uint64_t)interpreter->iregs[REG(a1)],
          interpreter->iregs[REG(a2)]);
      break;
    case RISC_V_ECALL_THREAD_SLEEP:
      interpreter->iregs[REG(a0)] = (uint64_t)RISCVSyscallThreadSleep(
          interpreter->guest_thread, (uint64_t)interpreter->iregs[REG(a1)],
          (uint64_t)interpreter->iregs[REG(a2)]);
      break;
    case RISC_V_ECALL_HARDWARE_CONCURRENCY:
      interpreter->iregs[REG(a0)] =
          (uint64_t)RISCVSyscallHardwareConcurrency();
      break;
    case RISC_V_ECALL_REALTIME_TIME: {
      struct timespec now;
      int64_t* result = (int64_t*)RISCVGuestAddressToHost(
          interpreter, (uint64_t)interpreter->iregs[REG(a1)], sizeof(int64_t));
      if (result == NULL || clock_gettime(CLOCK_REALTIME, &now) != 0) {
        interpreter->iregs[REG(a0)] = (uint64_t)-1;
      } else {
        *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
        interpreter->iregs[REG(a0)] = 0;
      }
      break;
    }
    case RISC_V_ECALL_FS_STATUS: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      DaveHostFilesystemStat* result =
          (DaveHostFilesystemStat*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a3)],
              sizeof(DaveHostFilesystemStat));
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostFilesystemGetStatus(
          path, (int)interpreter->iregs[REG(a2)], result);
      break;
    }
    case RISC_V_ECALL_FS_OPEN_DIRECTORY: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemOpenDirectory(path);
      break;
    }
    case RISC_V_ECALL_FS_READ_DIRECTORY: {
      DaveHostFilesystemDirectoryEntry* result =
          (DaveHostFilesystemDirectoryEntry*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a2)],
              sizeof(DaveHostFilesystemDirectoryEntry));
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemReadDirectory(
              (int)interpreter->iregs[REG(a1)], result);
      break;
    }
    case RISC_V_ECALL_FS_CLOSE_DIRECTORY:
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemCloseDirectory(
              (int)interpreter->iregs[REG(a1)]);
      break;
    case RISC_V_ECALL_FS_CREATE_DIRECTORY: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemCreateDirectory(
              path, (uint32_t)interpreter->iregs[REG(a2)]);
      break;
    }
    case RISC_V_ECALL_FS_REMOVE: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemRemove(path);
      break;
    }
    case RISC_V_ECALL_FS_RENAME: {
      const char* old_path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      const char* new_path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemRename(old_path, new_path);
      break;
    }
    case RISC_V_ECALL_FS_CURRENT_PATH: {
      size_t capacity = (size_t)interpreter->iregs[REG(a2)];
      char* buffer = (char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], capacity);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemCurrentPath(buffer, capacity);
      break;
    }
    case RISC_V_ECALL_FS_SET_CURRENT_PATH: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemSetCurrentPath(path);
      break;
    }
    case RISC_V_ECALL_FS_READ_SYMLINK: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      size_t capacity = (size_t)interpreter->iregs[REG(a3)];
      char* buffer = (char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], capacity);
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostFilesystemReadSymlink(
          path, buffer, capacity);
      break;
    }
    case RISC_V_ECALL_FS_CREATE_SYMLINK:
    case RISC_V_ECALL_FS_CREATE_HARD_LINK: {
      const char* target = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      const char* link = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], 1);
      int64_t result =
          interpreter->iregs[REG(t6)] == RISC_V_ECALL_FS_CREATE_SYMLINK
              ? DaveHostFilesystemCreateSymlink(target, link)
              : DaveHostFilesystemCreateHardLink(target, link);
      interpreter->iregs[REG(a0)] = (uint64_t)result;
      break;
    }
    case RISC_V_ECALL_FS_SET_PERMISSIONS: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemSetPermissions(
              path, (uint32_t)interpreter->iregs[REG(a2)],
              (int)interpreter->iregs[REG(a3)]);
      break;
    }
    case RISC_V_ECALL_FS_RESIZE: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      uint64_t* size = (uint64_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], sizeof(uint64_t));
      interpreter->iregs[REG(a0)] =
          size == NULL ? (uint64_t)-(int64_t)DAVE_HOST_EINVAL
                       : (uint64_t)DaveHostFilesystemResize(path, *size);
      break;
    }
    case RISC_V_ECALL_FS_SET_MODIFICATION_TIME: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      int64_t* nanoseconds = (int64_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], sizeof(int64_t));
      interpreter->iregs[REG(a0)] =
          nanoseconds == NULL
              ? (uint64_t)-(int64_t)DAVE_HOST_EINVAL
              : (uint64_t)DaveHostFilesystemSetModificationTime(
                    path, *nanoseconds);
      break;
    }
    case RISC_V_ECALL_FS_SPACE: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      DaveHostFilesystemSpace* result =
          (DaveHostFilesystemSpace*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a2)],
              sizeof(DaveHostFilesystemSpace));
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemQuerySpace(path, result);
      break;
    }
    case RISC_V_ECALL_FS_COPY_FILE: {
      const char* source = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      const char* destination = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], 1);
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostFilesystemCopyFile(
          source, destination, (int)interpreter->iregs[REG(a3)]);
      break;
    }
    case RISC_V_ECALL_FS_CANONICAL: {
      const char* path = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      size_t capacity = (size_t)interpreter->iregs[REG(a3)];
      char* buffer = (char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], capacity);
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemCanonical(path, buffer, capacity);
      break;
    }
    case RISC_V_ECALL_FS_DESCRIPTOR_STATUS: {
      DaveHostFilesystemStat* result =
          (DaveHostFilesystemStat*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a2)],
              sizeof(DaveHostFilesystemStat));
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostFilesystemGetDescriptorStatus(
              (int)interpreter->iregs[REG(a1)], result);
      break;
    }
    case RISC_V_ECALL_ENVIRONMENT_VALUE: {
      const char* name = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      size_t capacity = (size_t)interpreter->iregs[REG(a3)];
      char* buffer = (char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)], capacity);
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostEnvironmentValue(
          name, buffer, capacity);
      break;
    }
    case RISC_V_ECALL_TZDB_VERSION:
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostChronoTzdbVersion(
          (char*)RISCVGuestAddressToHost(interpreter, interpreter->iregs[REG(a1)],
                                         (size_t)interpreter->iregs[REG(a2)]),
          (size_t)interpreter->iregs[REG(a2)]);
      break;
    case RISC_V_ECALL_TZDB_GENERATION: {
      uint64_t* generation = (uint64_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], sizeof(uint64_t));
      interpreter->iregs[REG(a0)] =
          generation == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostChronoGeneration(generation);
      break;
    }
    case RISC_V_ECALL_TZDB_RELOAD: {
      uint64_t* generation = (uint64_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], sizeof(uint64_t));
      interpreter->iregs[REG(a0)] =
          (uint64_t)DaveHostChronoReload(generation);
      break;
    }
    case RISC_V_ECALL_TZDB_CURRENT_ZONE:
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostChronoCurrentZone(
          (char*)RISCVGuestAddressToHost(interpreter, interpreter->iregs[REG(a1)],
                                         (size_t)interpreter->iregs[REG(a2)]),
          (size_t)interpreter->iregs[REG(a2)]);
      break;
    case RISC_V_ECALL_TZDB_ZONE_COUNT: {
      uint32_t* count = (uint32_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], sizeof(uint32_t));
      interpreter->iregs[REG(a0)] =
          count == NULL ? (uint64_t)-DAVE_HOST_EINVAL
                        : (uint64_t)DaveHostChronoZoneCount(count);
      break;
    }
    case RISC_V_ECALL_TZDB_ZONE_NAME:
      interpreter->iregs[REG(a0)] = (uint64_t)DaveHostChronoZoneName(
          (uint32_t)interpreter->iregs[REG(a1)],
          (char*)RISCVGuestAddressToHost(interpreter, interpreter->iregs[REG(a2)],
                                         (size_t)interpreter->iregs[REG(a3)]),
          (size_t)interpreter->iregs[REG(a3)]);
      break;
    case RISC_V_ECALL_TZDB_LOCATE_ZONE: {
      uint32_t* index = (uint32_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a4)], sizeof(uint32_t));
      interpreter->iregs[REG(a0)] =
          index == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostChronoLocateZone(
                    (const char*)RISCVGuestAddressToHost(
                        interpreter, interpreter->iregs[REG(a1)], 1),
                    (char*)RISCVGuestAddressToHost(
                        interpreter, interpreter->iregs[REG(a2)],
                        (size_t)interpreter->iregs[REG(a3)]),
                    (size_t)interpreter->iregs[REG(a3)], index);
      break;
    }
    case RISC_V_ECALL_TZDB_SYS_INFO: {
      const char* zone = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      const DaveHostChronoSysInfoRequestWire* request =
          (const DaveHostChronoSysInfoRequestWire*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a2)],
              sizeof(DaveHostChronoSysInfoRequestWire));
      interpreter->iregs[REG(a0)] =
          zone == NULL || request == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostChronoSysInfoRequest(zone, request);
      break;
    }
    case RISC_V_ECALL_TZDB_LOCAL_INFO: {
      const char* zone = (const char*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], 1);
      const DaveHostChronoLocalInfoRequestWire* request =
          (const DaveHostChronoLocalInfoRequestWire*)RISCVGuestAddressToHost(
              interpreter, interpreter->iregs[REG(a2)],
              sizeof(DaveHostChronoLocalInfoRequestWire));
      interpreter->iregs[REG(a0)] =
          zone == NULL || request == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostChronoLocalInfoRequest(zone, request);
      break;
    }
    case RISC_V_ECALL_TZDB_LEAP_COUNT: {
      uint32_t* count = (uint32_t*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a1)], sizeof(uint32_t));
      interpreter->iregs[REG(a0)] =
          count == NULL ? (uint64_t)-DAVE_HOST_EINVAL
                        : (uint64_t)DaveHostChronoLeapCount(count);
      break;
    }
    case RISC_V_ECALL_TZDB_LEAP_INFO: {
      DaveHostChronoLeapSecond* leap = (DaveHostChronoLeapSecond*)RISCVGuestAddressToHost(
          interpreter, interpreter->iregs[REG(a2)],
          sizeof(DaveHostChronoLeapSecond));
      interpreter->iregs[REG(a0)] =
          leap == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostChronoLeapInfo(
                    (uint32_t)interpreter->iregs[REG(a1)], leap);
      break;
    }
    case RISC_V_ECALL_RANDOM_BYTES: {
      size_t size = (size_t)interpreter->iregs[REG(a2)];
      void* buffer =
          size == 0
              ? NULL
              : RISCVGuestAddressToHost(
                    interpreter, interpreter->iregs[REG(a1)], size);
      interpreter->iregs[REG(a0)] =
          size != 0 && buffer == NULL
              ? (uint64_t)-DAVE_HOST_EINVAL
              : (uint64_t)DaveHostRandomBytes(buffer, size);
      break;
    }
    default:
      DumpStateAndExit(interpreter);
  }
}

static void HandleEbreak(RISCVInterpreter* interpreter) {
  if (kDumpRegsonEbreak) {
    RISCVInterpreterDumpRegisters(interpreter);
  }
  longjmp(interpreter->debugger, 1);
}

static void DumpRegChanges(RISCVInterpreter* interpreter) {
  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    if (interpreter->iregs[i] != interpreter->old_iregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeInt, "");
      printf(": %08" PRIx64 " -> %08" PRIx64 "\n", interpreter->old_iregs[i],
             interpreter->iregs[i]);
    }
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    if (interpreter->fregs[i] != interpreter->old_fregs[i]) {
      DisassemblePrintRegister(stdout, i, kRegTypeFloat, "");
      printf(": %g -> %g\n", interpreter->old_fregs[i], interpreter->fregs[i]);
    }
  }
}

static uint64_t SetupGuestMainArgs(RISCVInterpreter* interpreter, int argc,
                                   char** argv) {
  uint64_t stack_base = interpreter->stack_guest_base != 0
                            ? interpreter->stack_guest_base
                            : (uint64_t)(uintptr_t)interpreter->stack;
  uint64_t stack_top = (stack_base + RISC_V_STACK_SIZE) & ~0xFULL;
  size_t pointer_size = interpreter->xlen32 ? 4 : 8;
  interpreter->iregs[RISC_V_REG_a0] = argc;
  if (argc <= 0 || argv == NULL) {
    interpreter->iregs[RISC_V_REG_a1] = 0;
    return stack_top;
  }

  size_t string_bytes = 0;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    if (len > RISC_V_STACK_SIZE - string_bytes) {
      goto invalid_args;
    }
    string_bytes += len;
  }
  if ((size_t)argc >= RISC_V_STACK_SIZE / pointer_size) {
    goto invalid_args;
  }
  size_t vector_bytes = (size_t)(argc + 1) * pointer_size;
  if (string_bytes + vector_bytes + 15 > RISC_V_STACK_SIZE) {
    goto invalid_args;
  }

  uint64_t string_address = stack_top - string_bytes;
  uint64_t guest_argv = (string_address - vector_bytes) & ~0xFULL;
  char* string_out = (char*)GuestMem(interpreter, (int64_t)string_address,
                                     string_bytes);
  char* pointer_out = (char*)GuestMem(interpreter, (int64_t)guest_argv,
                                      vector_bytes);
  uint64_t guest_str = string_address;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    memcpy(string_out, argv[i], len);
    if (interpreter->xlen32) {
      ((uint32_t*)pointer_out)[i] = (uint32_t)guest_str;
    } else {
      ((uint64_t*)pointer_out)[i] = guest_str;
    }
    string_out += len;
    guest_str += len;
  }
  if (interpreter->xlen32) {
    ((uint32_t*)pointer_out)[argc] = 0;
  } else {
    ((uint64_t*)pointer_out)[argc] = 0;
  }
  interpreter->iregs[RISC_V_REG_a1] = (int64_t)guest_argv;
  return guest_argv;

invalid_args:
  interpreter->iregs[RISC_V_REG_a0] = 0;
  interpreter->iregs[RISC_V_REG_a1] = 0;
  return stack_top;
}

void RISCVInterpreterInitForThread(
    RISCVInterpreter* interpreter, RISCVProcessRuntime* process,
    RISCVGuestThread* guest_thread, Loader* loader, uint64_t entry_address,
    int argc, char** argv, char* stack, void* tls_block,
    size_t tls_block_size, bool trace_regs, bool trace_instructions) {
  memset(interpreter, 0, sizeof(RISCVInterpreter));
  interpreter->process = process;
  interpreter->guest_thread = guest_thread;
  interpreter->trace_regs = trace_regs;
  interpreter->trace_instructions = trace_instructions;
  interpreter->num_steps = -1;

  // Create symbol resolver code.  This is invoked from the first
  // PLT entry with the following registers set:
  // t0: address of resolver data in the GOT.
  // t1: index into PLT for function to be called.
  //
  // The resolver data in the GOT contains:
  // [0]: Address of this code.
  // [1]: Address of LoadedDynamicLibrary containing the GOT and PLT
  //
  // This loads x10 with the ecall opcode (RISC_V_ECALL_RESOLVE)
  // and invokes an ecall instruction.
  interpreter->symbol_resolver_code[0] =
      RV_OPCODE(op_imm) | (31 << 7) |
      (RISC_V_ECALL_RESOLVE << 20);                          // addi x31, x0, 6
  interpreter->symbol_resolver_code[1] = RV_OPCODE(system);  // ecall

  interpreter->call_return_code[0] =
      RV_OPCODE(op_imm) | (31 << 7) |
      (RISC_V_ECALL_NESTED_RETURN << 20);
  interpreter->call_return_code[1] = RV_OPCODE(system);

  interpreter->loader = loader;
  interpreter->xlen32 = loader != NULL && loader->elf_file != NULL &&
                        loader->elf_file->ops != NULL &&
                        !loader->elf_file->ops->is_64_bit;
  interpreter->stack = stack;
  if (interpreter->stack == NULL) {
    interpreter->stack = malloc(RISC_V_STACK_SIZE);
    interpreter->owns_stack = true;
  }
  interpreter->stack_guest_base = interpreter->xlen32
                                      ? RISC_V_STACK_BASE
                                      : (uint64_t)(uintptr_t)interpreter->stack;
  if (tls_block != NULL) {
    interpreter->tls_block = tls_block;
    interpreter->tls_block_size = tls_block_size;
    interpreter->tls_guest_base = interpreter->xlen32
                                      ? RISC_V_TLS_BASE
                                      : (uint64_t)(uintptr_t)tls_block;
    interpreter->iregs[RISC_V_REG_tp] =
        (int64_t)interpreter->tls_guest_base;
  } else if (loader->tls.present) {
    interpreter->tls_block = loader->tls.main_thread_block;
    interpreter->tls_block_size = loader->tls.block_size;
    interpreter->tls_guest_base = interpreter->xlen32
                                      ? RISC_V_TLS_BASE
                                      : (uint64_t)(uintptr_t)
                                            loader->tls.main_thread_block;
    interpreter->iregs[RISC_V_REG_tp] = interpreter->xlen32
        ? (int64_t)interpreter->tls_guest_base
        : (int64_t)loader->tls.tp_base;
  }

  int64_t* iregs = interpreter->iregs;

  // Invoke interpreter at startup code.  This will call main and then
  // halt.
  int32_t* startup = interpreter->startup_code;
  // Startup code is:
  // x1 = entry_address
  // jalr x1, x1, 0
  // mv x10, 1
  // ecall
  startup[0] = RV_OPCODE(jalr) | (1 << 7) | (1 << 15);     // jalr x1, x1 ,0
  startup[1] = RV_OPCODE(op_imm) | (31 << 7) | (1 << 20);  // addi x31, x0, 1
  startup[2] = RV_OPCODE(system);                          // ecall
  if (interpreter->xlen32) {
    uint64_t linked = entry_address;
    LoaderRuntimeAddressToLinked(loader, entry_address, &linked);
    memcpy(interpreter->stack, startup, sizeof(interpreter->startup_code));
    memcpy(interpreter->stack + sizeof(interpreter->startup_code),
           interpreter->call_return_code,
           sizeof(interpreter->call_return_code));
    interpreter->iregs[1] = (int64_t)linked;
    interpreter->pc = (int64_t)interpreter->stack_guest_base;
  } else {
    interpreter->iregs[1] = entry_address;
    interpreter->pc = (int64_t)startup;
  }
  interpreter->running = true;
  interpreter->exit_code = 0;
  uint64_t argument_bottom = SetupGuestMainArgs(interpreter, argc, argv);
  iregs[RV_SP_REG] = (int64_t)(argument_bottom & ~0xFULL);

  if (kShowRegChanges) {
    interpreter->trace_regs = true;
  }
}

void RISCVInterpreterInit(RISCVInterpreter* interpreter, Loader* loader,
                          uint64_t entry_address, int argc,
                          char** argv, bool trace_regs,
                          bool trace_instructions) {
  RISCVInterpreterInitForThread(interpreter, NULL, NULL, loader,
                                entry_address, argc, argv, NULL, NULL, 0,
                                trace_regs, trace_instructions);
}

static uint64_t VectorLaneU(const uint8_t* vreg, int i, int sew) {
  uint64_t value = 0;
  memcpy(&value, vreg + (size_t)i * (size_t)sew, (size_t)sew);
  return value;
}

static int64_t VectorLaneS(const uint8_t* vreg, int i, int sew) {
  uint64_t value = VectorLaneU(vreg, i, sew);
  int shift = 64 - 8 * sew;
  return (int64_t)(value << shift) >> shift;
}

static void VectorSetLane(uint8_t* vreg, int i, int sew, uint64_t value) {
  memcpy(vreg + (size_t)i * (size_t)sew, &value, (size_t)sew);
}

static int VectorWidthBytes(int width) {
  switch (width) {
    case 0:
      return 1;
    case 5:
      return 2;
    case 6:
      return 4;
    case 7:
      return 8;
    default:
      return 0;
  }
}

static bool VectorMaskBit(const uint8_t* v0, int i) {
  return (v0[i / 8] >> (i % 8)) & 1;
}

static void VectorSetMaskBit(uint8_t* v0, int i, bool value) {
  uint8_t bit = (uint8_t)(1u << (i % 8));
  if (value) {
    v0[i / 8] |= bit;
  } else {
    v0[i / 8] &= (uint8_t)~bit;
  }
}

static void ExecuteVectorMem(RISCVInterpreter* interpreter, int32_t inst,
                             bool is_load) {
  int vd = (inst >> 7) & 0x1f;
  int rs1 = (inst >> 15) & 0x1f;
  int width = (inst >> 12) & 0x7;
  int mop = (inst >> 26) & 0x3;
  int lumop = (inst >> 20) & 0x1f;
  int sew = VectorWidthBytes(width);
  if (mop != 0 || lumop != 0 || sew == 0) {
    fprintf(stderr, "Unsupported vector memory instruction 0x%08x\n", inst);
    DumpStateAndExit(interpreter);
  }
  int vl = interpreter->vl;
  size_t bytes = (size_t)vl * (size_t)sew;
  if (bytes > RV_VLEN_BYTES) {
    bytes = RV_VLEN_BYTES;
  }
  uint8_t* host = (uint8_t*)(uintptr_t)interpreter->iregs[rs1];
  GuestMemoryLock(interpreter);
  if (is_load) {
    memcpy(interpreter->vregs[vd], host, bytes);
  } else {
    memcpy(host, interpreter->vregs[vd], bytes);
    GuestMemoryDidWrite(interpreter);
  }
  GuestMemoryUnlock(interpreter);
}

static void ExecuteOPV(RISCVInterpreter* interpreter, int32_t inst) {
  int vd = (inst >> 7) & 0x1f;
  int funct3 = (inst >> 12) & 0x7;
  int vs1 = (inst >> 15) & 0x1f;
  int vs2 = (inst >> 20) & 0x1f;
  int vm = (inst >> 25) & 1;
  int funct6 = (inst >> 26) & 0x3f;

  if (funct3 == 7) {
    if ((inst & (1u << 31)) == 0) {
      fprintf(stderr, "Unsupported vsetvli 0x%08x\n", inst);
      DumpStateAndExit(interpreter);
    }
    int avl = vs1;
    int vtype = (inst >> 20) & 0x7ff;
    int sew_log = (vtype >> 3) & 7;
    int sew = 1 << sew_log;
    int vlmax = RV_VLEN_BYTES / sew;
    int vl = avl < vlmax ? avl : vlmax;
    interpreter->vl = vl;
    interpreter->sew_bytes = sew;
    if (vd != 0) {
      interpreter->iregs[vd] = vl;
    }
    return;
  }

  int sew = interpreter->sew_bytes;
  int vl = interpreter->vl;
  if (sew != 1 && sew != 2 && sew != 4 && sew != 8) {
    fprintf(stderr, "Invalid SEW for vector op 0x%08x\n", inst);
    DumpStateAndExit(interpreter);
  }
  int lanes = vl;
  if (lanes * sew > RV_VLEN_BYTES) {
    lanes = RV_VLEN_BYTES / sew;
  }

  uint8_t* dest = interpreter->vregs[vd];
  const uint8_t* left = interpreter->vregs[vs2];
  const uint8_t* right = interpreter->vregs[vs1];

  if (funct3 == 3 && funct6 == 0x17) {
    int64_t imm = (int64_t)(int32_t)(vs1 << 27) >> 27;
    if (vm) {
      for (int i = 0; i < lanes; i++) {
        VectorSetLane(dest, i, sew, (uint64_t)imm);
      }
    } else {
      uint8_t result[RV_VLEN_BYTES];
      memcpy(result, dest, RV_VLEN_BYTES);
      for (int i = 0; i < lanes; i++) {
        uint64_t value = VectorMaskBit(interpreter->vregs[0], i)
                             ? (uint64_t)imm
                             : VectorLaneU(left, i, sew);
        VectorSetLane(result, i, sew, value);
      }
      memcpy(dest, result, RV_VLEN_BYTES);
    }
    return;
  }

  uint8_t result[RV_VLEN_BYTES];
  memcpy(result, dest, RV_VLEN_BYTES);

  if (funct3 == 0) {
    bool is_compare = funct6 >= 0x18 && funct6 <= 0x1d;
    if (is_compare) {
      memset(result, 0, RV_VLEN_BYTES);
      for (int i = 0; i < lanes; i++) {
        bool cond = false;
        uint64_t a = VectorLaneU(left, i, sew);
        uint64_t b = VectorLaneU(right, i, sew);
        int64_t sa = VectorLaneS(left, i, sew);
        int64_t sb = VectorLaneS(right, i, sew);
        switch (funct6) {
          case 0x18:
            cond = a == b;
            break;
          case 0x19:
            cond = a != b;
            break;
          case 0x1a:
            cond = a < b;
            break;
          case 0x1b:
            cond = sa < sb;
            break;
          case 0x1c:
            cond = a <= b;
            break;
          case 0x1d:
            cond = sa <= sb;
            break;
        }
        VectorSetMaskBit(result, i, cond);
      }
      memcpy(dest, result, RV_VLEN_BYTES);
      return;
    }
    for (int i = 0; i < lanes; i++) {
      uint64_t a = VectorLaneU(left, i, sew);
      uint64_t b = VectorLaneU(right, i, sew);
      int64_t sa = VectorLaneS(left, i, sew);
      int64_t sb = VectorLaneS(right, i, sew);
      uint64_t r = 0;
      int shift_mask = sew * 8 - 1;
      switch (funct6) {
        case 0x00:
          r = a + b;
          break;
        case 0x02:
          r = a - b;
          break;
        case 0x09:
          r = a & b;
          break;
        case 0x0a:
          r = a | b;
          break;
        case 0x0b:
          r = a ^ b;
          break;
        case 0x25:
          r = a << (b & (uint64_t)shift_mask);
          break;
        case 0x28:
          r = a >> (b & (uint64_t)shift_mask);
          break;
        case 0x29:
          r = (uint64_t)(sa >> (sb & shift_mask));
          break;
        default:
          fprintf(stderr, "Unsupported OPIVV 0x%08x\n", inst);
          DumpStateAndExit(interpreter);
      }
      VectorSetLane(result, i, sew, r);
    }
    memcpy(dest, result, RV_VLEN_BYTES);
    return;
  }

  if (funct3 == 2) {
    for (int i = 0; i < lanes; i++) {
      uint64_t a = VectorLaneU(left, i, sew);
      uint64_t b = VectorLaneU(right, i, sew);
      int64_t sa = VectorLaneS(left, i, sew);
      int64_t sb = VectorLaneS(right, i, sew);
      uint64_t r = 0;
      switch (funct6) {
        case 0x25:
          r = a * b;
          break;
        case 0x20:
          r = b == 0 ? (uint64_t)-1 : a / b;
          break;
        case 0x21:
          r = sb == 0 ? (uint64_t)-1 : (uint64_t)(sa / sb);
          break;
        case 0x22:
          r = b == 0 ? a : a % b;
          break;
        case 0x23:
          r = sb == 0 ? (uint64_t)sa : (uint64_t)(sa % sb);
          break;
        default:
          fprintf(stderr, "Unsupported OPMVV 0x%08x\n", inst);
          DumpStateAndExit(interpreter);
      }
      VectorSetLane(result, i, sew, r);
    }
    memcpy(dest, result, RV_VLEN_BYTES);
    return;
  }

  if (funct3 == 1) {
    for (int i = 0; i < lanes; i++) {
      if (sew == 8) {
        double a, b, r;
        memcpy(&a, left + i * 8, 8);
        memcpy(&b, right + i * 8, 8);
        switch (funct6) {
          case 0x00:
            r = a + b;
            break;
          case 0x02:
            r = a - b;
            break;
          case 0x24:
            r = a * b;
            break;
          case 0x20:
            r = a / b;
            break;
          default:
            fprintf(stderr, "Unsupported OPFVV 0x%08x\n", inst);
            DumpStateAndExit(interpreter);
        }
        memcpy(result + i * 8, &r, 8);
      } else {
        float a, b, r;
        memcpy(&a, left + i * 4, 4);
        memcpy(&b, right + i * 4, 4);
        switch (funct6) {
          case 0x00:
            r = a + b;
            break;
          case 0x02:
            r = a - b;
            break;
          case 0x24:
            r = a * b;
            break;
          case 0x20:
            r = a / b;
            break;
          default:
            fprintf(stderr, "Unsupported OPFVV 0x%08x\n", inst);
            DumpStateAndExit(interpreter);
        }
        memcpy(result + i * 4, &r, 4);
      }
    }
    memcpy(dest, result, RV_VLEN_BYTES);
    return;
  }

  fprintf(stderr, "Unsupported OP-V 0x%08x\n", inst);
  DumpStateAndExit(interpreter);
}

void RISCVInterpreterPrepareMain(RISCVInterpreter* interpreter,
                                 uint64_t entry_address, int argc,
                                 char** argv) {
  if (interpreter->xlen32) {
    uint64_t linked = entry_address;
    LoaderRuntimeAddressToLinked(interpreter->loader, entry_address, &linked);
    interpreter->iregs[RISC_V_REG_ra] = (int64_t)linked;
    interpreter->pc = (int64_t)interpreter->stack_guest_base;
  } else {
    interpreter->iregs[RISC_V_REG_ra] = (int64_t)entry_address;
    interpreter->pc = (int64_t)interpreter->startup_code;
  }
  interpreter->running = true;
  interpreter->exit_code = 0;
  uint64_t argument_bottom = SetupGuestMainArgs(interpreter, argc, argv);
  interpreter->iregs[RV_SP_REG] = (int64_t)(argument_bottom & ~0xFULL);
}

void RISCVInterpreterCycle(RISCVInterpreter* interpreter) {
  int64_t* iregs = interpreter->iregs;
  double* fregs = interpreter->fregs;
  for (; interpreter->pc != 0;) {
    if (interpreter->num_steps > 0) {
      --interpreter->num_steps;
    }

    if (interpreter->trace_instructions) {
      interpreter->current_symbol =
          LoaderFindSymbolAndCacheResult(interpreter->loader, interpreter->pc);
    }

    // x0 is hardcoded as zero.  Reset it every loop in case it's been
    // overwritten.
    iregs[RV_INT_ZERO_REG] = 0;

    if (interpreter->trace_regs) {
      memcpy(interpreter->old_iregs, interpreter->iregs,
             sizeof(interpreter->iregs));
      memcpy(interpreter->old_fregs, interpreter->fregs,
             sizeof(interpreter->fregs));
    }
    // Disassemble unless it's an ebreak instruction.
    bool is_ebreak = FetchInst(interpreter) == ((1 << 20) | 0x73);
    if (!is_ebreak && interpreter->trace_instructions) {
      uint32_t* ip = GuestMem(interpreter, interpreter->pc, 4);
      DisassembleRiscVInstruction(interpreter, ip, stdout);
    }

    int32_t inst = (int32_t)FetchInst(interpreter);  // Signed 32 bits.
    RVInstOpcode opcode = inst & 0x7f;
    int rd = (inst >> 7) & 0x1f;
    int rs1 = (inst >> 15) & 0x1f;
    int rs2 = (inst >> 20) & 0x1f;
    switch (opcode) {
      case RV_OPCODE(op): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        if (funct7 == RV_F7(mul)) {
          switch (funct3) {
            case RV_F3(mul):
              iregs[rd] = iregs[rs1] * iregs[rs2];
              break;
            case RV_F3(mulh): {
              if (interpreter->xlen32) {
                iregs[rd] = (int32_t)(((int64_t)(int32_t)iregs[rs1] *
                                       (int64_t)(int32_t)iregs[rs2]) >>
                                      32);
                break;
              }
              // From:
              // https://stackoverflow.com/questions/28868367/getting-the-high-part-of-64-bit-integer-multiplication
              int64_t a = iregs[rs1];
              int64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(mulhsu): {
              if (interpreter->xlen32) {
                iregs[rd] = (int32_t)(((int64_t)(int32_t)iregs[rs1] *
                                       (int64_t)(uint32_t)iregs[rs2]) >>
                                      32);
                break;
              }
              int64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(mulhu): {
              if (interpreter->xlen32) {
                iregs[rd] = (int32_t)(((uint64_t)(uint32_t)iregs[rs1] *
                                       (uint64_t)(uint32_t)iregs[rs2]) >>
                                      32);
                break;
              }
              uint64_t a = iregs[rs1];
              uint64_t b = iregs[rs2];
              uint64_t a_lo = (uint32_t)a;
              uint64_t a_hi = a >> 32;
              uint64_t b_lo = (uint32_t)b;
              uint64_t b_hi = b >> 32;

              uint64_t a_x_b_hi = a_hi * b_hi;
              uint64_t a_x_b_mid = a_hi * b_lo;
              uint64_t b_x_a_mid = b_hi * a_lo;
              uint64_t a_x_b_lo = a_lo * b_lo;

              uint64_t carry_bit =
                  ((uint64_t)(uint32_t)a_x_b_mid +
                   (uint64_t)(uint32_t)b_x_a_mid + (a_x_b_lo >> 32)) >>
                  32;

              uint64_t multhi =
                  a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;

              iregs[rd] = multhi;
              break;
            }
            case RV_F3(div):
              iregs[rd] = iregs[rs1] / iregs[rs2];
              break;
            case RV_F3(divu):
              iregs[rd] = (int64_t)(LogicalBits(interpreter, iregs[rs1]) /
                                    LogicalBits(interpreter, iregs[rs2]));
              break;
            case RV_F3(rem):
              iregs[rd] = iregs[rs1] % iregs[rs2];
              break;
            case RV_F3(remu):
              iregs[rd] = (int64_t)(LogicalBits(interpreter, iregs[rs1]) %
                                    LogicalBits(interpreter, iregs[rs2]));
              break;
          }
          break;
        }
        switch (funct3) {
          case RV_F3(add):  // add and sub:
            if (funct7 == RV_F7(sub)) {
              iregs[rd] = iregs[rs1] - iregs[rs2];
            } else {
              iregs[rd] = iregs[rs1] + iregs[rs2];
            }
            break;
          case RV_F3(sll):
            iregs[rd] = LogicalBits(interpreter, iregs[rs1])
                        << (iregs[rs2] & ShiftMask(interpreter));
            break;
          case RV_F3(slt):
            iregs[rd] = iregs[rs1] < iregs[rs2];
            break;
          case RV_F3(sltu):
            iregs[rd] = LogicalBits(interpreter, iregs[rs1]) <
                        LogicalBits(interpreter, iregs[rs2]);
            break;
          case RV_F3 (xor):
            iregs[rd] = iregs[rs1] ^ iregs[rs2];
            break;
          case RV_F3(srl):  // and sra
            if (funct7 == RV_F7(sra)) {
              iregs[rd] = iregs[rs1] >> (iregs[rs2] & ShiftMask(interpreter));
            } else {
              iregs[rd] = LogicalBits(interpreter, iregs[rs1]) >>
                          (iregs[rs2] & ShiftMask(interpreter));
            }
            break;

          case RV_F3(or):
            iregs[rd] = iregs[rs1] | iregs[rs2];
            break;
          case RV_F3(and):
            iregs[rd] = iregs[rs1] & iregs[rs2];
            break;
          default:
            break;
        }
        break;
      }
      case RV_OPCODE(op_imm): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        switch (funct3) {
          case RV_F3(addi):
            iregs[rd] = iregs[rs1] + immed;
            break;
          case RV_F3(slti):
            iregs[rd] = iregs[rs1] < immed;
            break;
          case RV_F3(sltiu):
            iregs[rd] = LogicalBits(interpreter, iregs[rs1]) <
                        LogicalBits(interpreter, immed);
            break;
          case RV_F3(xori):
            iregs[rd] = iregs[rs1] ^ immed;
            break;
          case RV_F3(ori):
            iregs[rd] = iregs[rs1] | immed;
            break;
          case RV_F3(andi):
            iregs[rd] = iregs[rs1] & immed;
            break;
          case RV_F3(slli): {
            int shamt = (inst >> 20) & ShiftMask(interpreter);
            iregs[rd] = LogicalBits(interpreter, iregs[rs1]) << shamt;
            break;
          }
          case RV_F3(srli): {
            // In R64 the shift amount is 6 bits and overlaps the F7 field
            // by one bit.  We need to clear this bottom bit before checking
            // the F7.
            int funct7 = (inst >> 25) & 0x7e;  // Bottom bit is cleared
            int shamt = (inst >> 20) & ShiftMask(interpreter);
            if (funct7 == RV_F7(srai)) {
              iregs[rd] = iregs[rs1] >> shamt;
            } else {
              iregs[rd] = LogicalBits(interpreter, iregs[rs1]) >> shamt;
            }
            break;
          }
          default:
            break;
        }
        break;
      }
      case RV_OPCODE(lui): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
        iregs[rd] = immed << 12;
        break;
      }
      case RV_OPCODE(auipc): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
        iregs[rd] = interpreter->pc + (immed << 12);
        break;
      }
      case RV_OPCODE(jal): {
        // immediate at bit 12 is encoded as imm[20|10:1|11|19:12]
        int64_t imm = inst >> 12;  // Auto sign extended to 64 bits.
        int64_t immed = (imm & 0xff) << 12 | ((imm >> 8) & 1) << 11 |
                        ((imm >> 9) & 0x3ff) << 1 | ((imm >> 19) & 1) << 20;
        // Sign extend to 64 bits.
        immed <<= 63 - 20;
        immed >>= 63 - 20;
        iregs[rd] = interpreter->pc + 4;
        interpreter->pc += immed - 4;
        break;
      }
      case RV_OPCODE(jalr): {
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        int64_t old_pc = interpreter->pc;
        interpreter->pc = iregs[rs1] + immed - 4;
        iregs[rd] = old_pc + 4;
        break;
      }
      case RV_OPCODE(branch): {
        int64_t hi = inst >> 25;
        int64_t offset = (rd & 0x1e) | ((rd & 1) << 11) | ((hi & 0x3f) << 5) |
                         ((hi >> 6) << 12);
        offset -= 4;  // Adjust for += 4 at end of loop.
        int funct3 = (inst >> 12) & 0x7;
        switch (funct3) {
          case RV_F3(beq):
            if (iregs[rs1] == iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bne):
            if (iregs[rs1] != iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(blt):
            if (iregs[rs1] < iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bge):
            if (iregs[rs1] >= iregs[rs2]) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bltu):
            if (LogicalBits(interpreter, iregs[rs1]) <
                LogicalBits(interpreter, iregs[rs2])) {
              interpreter->pc += offset;
            }
            break;
          case RV_F3(bgeu):
            if (LogicalBits(interpreter, iregs[rs1]) >=
                LogicalBits(interpreter, iregs[rs2])) {
              interpreter->pc += offset;
            }
            break;
        }
        break;
      }
      case RV_OPCODE(load): {
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        GuestMemoryLock(interpreter);
        switch (funct3) {
          case RV_F3(lb):
            iregs[rd] =
                *(int8_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 1);
            break;
          case RV_F3(lh):
            iregs[rd] =
                *(int16_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 2);
            break;
          case RV_F3(lw):
            iregs[rd] =
                *(int32_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 4);
            break;
          case RV_F3(lbu):
            iregs[rd] =
                *(uint8_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 1);
            break;
          case RV_F3(lhu):
            iregs[rd] = *(uint16_t*)RequireGuestMem(
                interpreter, iregs[rs1] + immed, 2);
            break;
          case RV_F3(lwu):
            if (interpreter->xlen32) {
              DumpStateAndExit(interpreter);
            }
            iregs[rd] = *(uint32_t*)RequireGuestMem(
                interpreter, iregs[rs1] + immed, 4);
            break;
          case RV_F3(ld):
            if (interpreter->xlen32) {
              DumpStateAndExit(interpreter);
            }
            iregs[rd] = *(uint64_t*)RequireGuestMem(
                interpreter, iregs[rs1] + immed, 8);
            break;
        }
        GuestMemoryUnlock(interpreter);
        break;
      }
      case RV_OPCODE(store): {
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
        int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
        GuestMemoryLock(interpreter);
        switch (funct3) {
          case RV_F3(sb):
            *(int8_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 1) =
                iregs[rs2];
            break;
          case RV_F3(sh):
            *(int16_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 2) =
                iregs[rs2];
            break;
          case RV_F3(sw):
            *(int32_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 4) =
                (int32_t)iregs[rs2];
            break;
          case RV_F3(sd):
            if (interpreter->xlen32) {
              DumpStateAndExit(interpreter);
            }
            *(uint64_t*)RequireGuestMem(interpreter, iregs[rs1] + immed, 8) =
                iregs[rs2];
            break;
        }
        GuestMemoryDidWrite(interpreter);
        GuestMemoryUnlock(interpreter);
        break;
      }
      case RV_OPCODE(amo): {
        int funct3 = (inst >> 12) & 0x7;
        int funct5 = (inst >> 27) & 0x1f;
        uint64_t address = (uint64_t)iregs[rs1];
        uint32_t size = funct3 == 2 ? 4 : 8;
        if (interpreter->xlen32 && size != 4) {
          DumpStateAndExit(interpreter);
        }
        GuestMemoryLock(interpreter);
        void* host = RequireGuestMem(interpreter, (int64_t)address, size);
        if (funct5 == 0x02) {
          iregs[rd] =
              size == 4 ? (int64_t)*(int32_t*)host
                        : (int64_t)*(int64_t*)host;
          interpreter->reservation_valid = true;
          interpreter->reservation_address = address;
          interpreter->reservation_size = size;
          interpreter->reservation_epoch =
              interpreter->process != NULL
                  ? interpreter->process->write_epoch
                  : 0;
        } else if (funct5 == 0x03) {
          bool success =
              interpreter->reservation_valid &&
              interpreter->reservation_address == address &&
              interpreter->reservation_size == size &&
              (interpreter->process == NULL ||
               interpreter->reservation_epoch ==
                   interpreter->process->write_epoch);
          interpreter->reservation_valid = false;
          if (success) {
            if (size == 4) {
              *(int32_t*)host = (int32_t)iregs[rs2];
            } else {
              *(int64_t*)host = iregs[rs2];
            }
            if (interpreter->process != NULL) {
              interpreter->process->write_epoch++;
            }
            iregs[rd] = 0;
          } else {
            iregs[rd] = 1;
          }
        } else if (funct5 == 0x00) {
          if (size == 4) {
            int32_t old = *(int32_t*)host;
            *(int32_t*)host = old + (int32_t)iregs[rs2];
            iregs[rd] = old;
          } else {
            int64_t old = *(int64_t*)host;
            *(int64_t*)host = old + iregs[rs2];
            iregs[rd] = old;
          }
          GuestMemoryDidWrite(interpreter);
        }
        GuestMemoryUnlock(interpreter);
        break;
      }
      case RV_OPCODE(misc_mem): {
        atomic_thread_fence(memory_order_seq_cst);
        break;
      }
      case RV_OPCODE(system): {
        int op = inst >> 20;
        switch (op) {
          case 0:  // ecall
            HandleEcall(interpreter);
            break;
          case 1:  // ebreak
            HandleEbreak(interpreter);
            break;
        }
        break;
      }
      case RV_OPCODE(op_imm_32): {
        if (interpreter->xlen32) {
          DumpStateAndExit(interpreter);
        }
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        switch (funct3) {
          case RV_F3(addiw):
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) + immed;
            break;
          case RV_F3(slliw): {
            int shamt = (inst >> 20) & 0x3f;
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) << shamt;
            break;
          }
          case RV_F3(srliw): {
            int funct7 = (inst >> 25) & 0x7f;
            int shamt = (inst >> 20) & 0x3f;
            if (funct7 == RV_F7(sraiw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) >> shamt;
            } else {
              iregs[rd] = (uint32_t)(iregs[rs1] & 0xffffffff) >> shamt;
            }
          }
        }
        break;
      }
      case RV_OPCODE(op_32): {
        if (interpreter->xlen32) {
          DumpStateAndExit(interpreter);
        }
        if (rd == 0) {
          // Writing to x0 is a nop.
          break;
        }
        int funct3 = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        if (funct7 == RV_F7(mulw)) {
          int32_t lhs = (int32_t)iregs[rs1];
          int32_t rhs = (int32_t)iregs[rs2];
          uint32_t ulhs = (uint32_t)iregs[rs1];
          uint32_t urhs = (uint32_t)iregs[rs2];
          switch (funct3) {
            case RV_F3(mulw):
              iregs[rd] = (int32_t)(ulhs * urhs);
              break;
            case RV_F3(divw):
              if (rhs == 0) {
                iregs[rd] = -1;
              } else if (lhs == INT32_MIN && rhs == -1) {
                iregs[rd] = INT32_MIN;
              } else {
                iregs[rd] = lhs / rhs;
              }
              break;
            case RV_F3(divuw):
              iregs[rd] = (int32_t)(urhs == 0 ? UINT32_MAX : ulhs / urhs);
              break;
            case RV_F3(remw):
              if (rhs == 0) {
                iregs[rd] = lhs;
              } else if (lhs == INT32_MIN && rhs == -1) {
                iregs[rd] = 0;
              } else {
                iregs[rd] = lhs % rhs;
              }
              break;
            case RV_F3(remuw):
              iregs[rd] = (int32_t)(urhs == 0 ? ulhs : ulhs % urhs);
              break;
          }
          break;
        }
        switch (funct3) {
          case RV_F3(addw):
            if (funct7 == RV_F7(subw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) -
                          (int32_t)(iregs[rs2] & 0xffffffff);
            } else {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) +
                          (int32_t)(iregs[rs2] & 0xffffffff);
            }
            break;
          case RV_F3(sllw):
            iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) << iregs[rs2];
            break;

          case RV_F3(srlw):
            if (funct7 == RV_F7(sraiw)) {
              iregs[rd] = (int32_t)(iregs[rs1] & 0xffffffff) >> iregs[rs2];
            } else {
              iregs[rd] = (uint32_t)(iregs[rs1] & 0xffffffff) >> iregs[rs2];
            }
            break;
        }
        break;
      }
      case RV_OPCODE(load_fp): {
        int funct3 = (inst >> 12) & 0x7;
        if (funct3 == 0 || funct3 == 5 || funct3 == 6 || funct3 == 7) {
          ExecuteVectorMem(interpreter, inst, true);
          break;
        }
        int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
        GuestMemoryLock(interpreter);
        if (funct3 == RV_F3(flw)) {
          fregs[rd] = *(float*)RequireGuestMem(
              interpreter, iregs[rs1] + immed, 4);
        } else {
          fregs[rd] = *(double*)RequireGuestMem(
              interpreter, iregs[rs1] + immed, 8);
        }
        GuestMemoryUnlock(interpreter);
        break;
      }
      case RV_OPCODE(store_fp): {
        int funct3 = (inst >> 12) & 0x7;
        if (funct3 == 0 || funct3 == 5 || funct3 == 6 || funct3 == 7) {
          ExecuteVectorMem(interpreter, inst, false);
          break;
        }
        int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
        int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
        GuestMemoryLock(interpreter);
        if (funct3 == RV_F3(fsw)) {
          *(float*)RequireGuestMem(interpreter, iregs[rs1] + immed, 4) =
              (float)fregs[rs2];
        } else {
          *(double*)RequireGuestMem(interpreter, iregs[rs1] + immed, 8) =
              fregs[rs2];
        }
        GuestMemoryDidWrite(interpreter);
        GuestMemoryUnlock(interpreter);
        break;
      }

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define MAX(x, y) (x) > (y) ? (x) : (y)

      // TODO: these don't use rounding mode.
      case RV_OPCODE(op_fp): {
        int rm = (inst >> 12) & 0x7;
        int funct7 = (inst >> 25) & 0x7f;
        switch (funct7) {
          case RV_F7(fadd_s):
            fregs[rd] = (float)fregs[rs1] + (float)fregs[rs2];
            break;
          case RV_F7(fsub_s):
            fregs[rd] = (float)fregs[rs1] - (float)fregs[rs2];
            break;
          case RV_F7(fmul_s):
            fregs[rd] = (float)fregs[rs1] * (float)fregs[rs2];
            break;
          case RV_F7(fdiv_s):
            fregs[rd] = (float)fregs[rs1] / (float)fregs[rs2];
            break;
          case RV_F7(fsqrt_s):
            fregs[rd] = (float)sqrt((float)fregs[rs1]);
            break;
          case RV_F7(fmin_s):
            if (rm == RV_F3(fmin_s)) {
              fregs[rd] = MIN((float)fregs[rs1], (float)fregs[rs2]);
            } else {
              fregs[rd] = MIN((float)fregs[rs1], (float)fregs[rs2]);
            }
            break;
          case RV_F7(fadd_d):
            fregs[rd] = fregs[rs1] + fregs[rs2];
            break;
          case RV_F7(fsub_d):
            fregs[rd] = fregs[rs1] - fregs[rs2];
            break;
          case RV_F7(fmul_d):
            fregs[rd] = fregs[rs1] * fregs[rs2];
            break;
          case RV_F7(fdiv_d):
            fregs[rd] = fregs[rs1] / fregs[rs2];
            break;
          case RV_F7(fsqrt_d):
            fregs[rd] = sqrt(fregs[rs1]);
            break;
          case RV_F7(fmin_d):
            if (rm == RV_F3(fmin_s)) {
              fregs[rd] = MIN(fregs[rs1], fregs[rs2]);
            } else {
              fregs[rd] = MIN(fregs[rs1], fregs[rs2]);
            }
            break;

          // Floating point comparisons.
          case RV_F7(feq_s):  // And flt.s, fle.s
            switch (rm) {
              case RV_F3(feq_s:)
                iregs[rd] = (float)fregs[rs1] == (float)fregs[rs2];
                break;
              case RV_F3(flt_s:)
                iregs[rd] = (float)fregs[rs1] < (float)fregs[rs2];
                break;
              case RV_F3(fle_s):
                iregs[rd] = (float)fregs[rs1] <= (float)fregs[rs2];
                break;
            }
            break;

          case RV_F7(feq_d):  // And flt.d, fle.d
            switch (rm) {
            case RV_F3(feq_d:)
              iregs[rd] = fregs[rs1] == fregs[rs2];
              break;
            case RV_F3(flt_d:)
              iregs[rd] = fregs[rs1] < fregs[rs2];
              break;
            case RV_F3(fle_d):
              iregs[rd] = fregs[rs1] <= fregs[rs2];
              break;
            }
            break;

          // Moves.
          case RV_F7(fmv_w_x): {
            uint32_t bits = (uint32_t)iregs[rs1];
            float value;
            memcpy(&value, &bits, sizeof(value));
            fregs[rd] = value;
            break;
          }

          case RV_F7(fmv_x_w): {
            float value = (float)fregs[rs1];
            uint32_t bits;
            memcpy(&bits, &value, sizeof(bits));
            iregs[rd] = (int32_t)bits;
            break;
          }

          case RV_F7(fmv_d_x):
            *(int64_t*)(&fregs[rd]) = iregs[rs1];
            break;

          case RV_F7(fmv_x_d):
            iregs[rd] = *(int64_t*)(&fregs[rs1]);
            break;

          case RV_F7(fcvt_s_w):  //  and RV_F7(fcvt_s_wu):
            switch (rs2) {
              case 0:
                fregs[rd] = (float)((int32_t)iregs[rs1]);
                break;
              case 1:
                fregs[rd] = (float)((uint32_t)iregs[rs1]);
                break;
              case 2:
                fregs[rd] = (float)((int64_t)iregs[rs1]);
                break;
              case 3:
                fregs[rd] = (float)((uint64_t)iregs[rs1]);
                break;
            }
            break;

          case RV_F7(fcvt_d_w):  // and RV_F7(fcvt_d_wu)/fcvt_d_l/fcvt_d_lu:
            switch (rs2) {
              case 0:
                fregs[rd] = (double)((int32_t)iregs[rs1]);
                break;
              case 1:
                fregs[rd] = (double)((uint32_t)iregs[rs1]);
                break;
              case 2:
                fregs[rd] = (double)((int64_t)iregs[rs1]);
                break;
              case 3:
                fregs[rd] = (double)((uint64_t)iregs[rs1]);
                break;
            }
            break;

          case RV_F7(fcvt_w_s):  // and RV_F7(fcvt_wu_s):
            switch (rs2) {
              case 0:
                iregs[rd] = (int32_t)fregs[rs1];
                break;
              case 1:
                iregs[rd] = (uint32_t)fregs[rs1];
                break;
              case 2:
                iregs[rd] = (int64_t)fregs[rs1];
                break;
              case 3:
                iregs[rd] = (uint64_t)fregs[rs1];
                break;
            }
            break;

          case RV_F7(fcvt_w_d):  // and RV_F7(fcvt_wu_d)/fcvt_l_d/fcvt_lu_d:
            switch (rs2) {
              case 0:
                iregs[rd] = (int32_t)fregs[rs1];
                break;
              case 1:
                iregs[rd] = (uint32_t)fregs[rs1];
                break;
              case 2:
                iregs[rd] = (int64_t)fregs[rs1];
                break;
              case 3:
                iregs[rd] = (uint64_t)fregs[rs1];
                break;
            }
            break;

          case RV_F7(fcvt_s_d):
            fregs[rd] = (float)fregs[rs1];
            break;

          case RV_F7(fcvt_d_s):
            fregs[rd] = (float)fregs[rs1];
            break;

          // TODO: These don't use rs2.
          case RV_F7(fsgnj_s):  // All sign injection instructions.
            switch (rm) {
              case RV_F3(fsgnj_s):
                fregs[rd] = fregs[rs1];
                break;
              case RV_F3(fsgnjn_s):
                fregs[rd] = -fregs[rs1];
                break;
              case RV_F3(fsgnjx_s):
                fregs[rd] = fabs(fregs[rs1]);
                break;
            }
            break;

          case RV_F7(fsgnj_d):  // All sign injection instructions.
            switch (rm) {
              case RV_F3(fsgnj_d):
                fregs[rd] = fregs[rs1];
                break;
              case RV_F3(fsgnjn_d):
                fregs[rd] = -fregs[rs1];
                break;
              case RV_F3(fsgnjx_d):
                fregs[rd] = fabs(fregs[rs1]);
                break;
            }
            break;
        }
        break;
      }
      case RV_OPCODE(op_v):
        ExecuteOPV(interpreter, inst);
        break;
      default:
        break;
    }

    if (interpreter->xlen32) {
      for (int i = 1; i < RV_NUM_INT_REGS; i++) {
        iregs[i] = (int32_t)iregs[i];
      }
    }

    if (interpreter->trace_regs) {
      DumpRegChanges(interpreter);
    }
    
    if (interpreter->num_steps == 0) {
      interpreter->pc += 4;
      break;
    }
    if (interpreter->pc == 0) {
      break;
    }
    interpreter->pc += 4;
  }
}

void RISCVInterpreterPrepareCall(RISCVInterpreter* interpreter, uint64_t fn) {
  interpreter->iregs[RISC_V_REG_ra] = (int64_t)GuestCallReturnPC(interpreter);
  interpreter->pc = (int64_t)GuestCallPC(interpreter, fn);
  interpreter->running = true;
}

typedef struct {
  int64_t pc;
  int64_t iregs[RV_NUM_INT_REGS];
  double fregs[RV_NUM_FLOAT_REGS];
  uint8_t vregs[RV_NUM_VECTOR_REGS][RV_VLEN_BYTES];
  int vl;
  int sew_bytes;
  bool running;
  int exit_code;
} RISCVSavedState;

static void RISCVInterpreterSaveState(RISCVInterpreter* interpreter,
                                      RISCVSavedState* saved) {
  saved->pc = interpreter->pc;
  memcpy(saved->iregs, interpreter->iregs, sizeof(saved->iregs));
  memcpy(saved->fregs, interpreter->fregs, sizeof(saved->fregs));
  memcpy(saved->vregs, interpreter->vregs, sizeof(saved->vregs));
  saved->vl = interpreter->vl;
  saved->sew_bytes = interpreter->sew_bytes;
  saved->running = interpreter->running;
  saved->exit_code = interpreter->exit_code;
}

static void RISCVInterpreterRestoreState(RISCVInterpreter* interpreter,
                                         const RISCVSavedState* saved) {
  interpreter->pc = saved->pc;
  memcpy(interpreter->iregs, saved->iregs, sizeof(interpreter->iregs));
  memcpy(interpreter->fregs, saved->fregs, sizeof(interpreter->fregs));
  memcpy(interpreter->vregs, saved->vregs, sizeof(interpreter->vregs));
  interpreter->vl = saved->vl;
  interpreter->sew_bytes = saved->sew_bytes;
  interpreter->running = saved->running;
  interpreter->exit_code = saved->exit_code;
}

void RISCVInterpreterCall(RISCVInterpreter* interpreter, uint64_t fn) {
  if (interpreter == NULL || fn == 0) {
    return;
  }
  RISCVSavedState saved;
  RISCVInterpreterSaveState(interpreter, &saved);
  RISCVInterpreterPrepareCall(interpreter, fn);
  RISCVInterpreterCycle(interpreter);
  RISCVInterpreterRestoreState(interpreter, &saved);
}

int RISCVInterpreterCallWithArg(RISCVInterpreter* interpreter, uint64_t fn,
                                uint64_t arg) {
  if (interpreter == NULL || fn == 0) {
    return 0;
  }
  RISCVSavedState saved;
  RISCVInterpreterSaveState(interpreter, &saved);
  RISCVInterpreterPrepareCall(interpreter, fn);
  interpreter->iregs[RISC_V_REG_a0] = (int64_t)arg;
  RISCVInterpreterCycle(interpreter);
  int result = (int)interpreter->iregs[RISC_V_REG_a0];
  if (interpreter->exit_code != 0) {
    result = interpreter->exit_code;
  }
  RISCVInterpreterRestoreState(interpreter, &saved);
  return result;
}

void* RISCVGuestAddressToHost(RISCVInterpreter* interpreter, uint64_t addr,
                              size_t size) {
  if (interpreter == NULL || addr == 0 || addr + size < addr) {
    return NULL;
  }
  if (interpreter->xlen32) {
    return GuestMem(interpreter, (int64_t)addr, size);
  }
  if (interpreter->process != NULL) {
    void* mapped =
        RISCVProcessResolveGuestAddress(interpreter->process, addr, size);
    if (mapped != NULL) {
      return mapped;
    }
  }
  if (interpreter->loader != NULL) {
    for (size_t i = 0; i < interpreter->loader->regions.length; i++) {
      Region* region = interpreter->loader->regions.value.p[i];
      uint64_t start = (uint64_t)(uintptr_t)region->address;
      uint64_t end = start + (uint64_t)region->length;
      if (addr >= start && addr + size <= end) {
        return (void*)(uintptr_t)addr;
      }
    }
  }
  return NULL;
}

int RISCVInterpreterRun(RISCVInterpreter* interpreter) {
  RISCVInterpreterCycle(interpreter);
  return interpreter->exit_code;
}

bool RISCVGuestAddressExecutable(Loader* loader, uint64_t addr) {
  if (AddressInExecutableRegion(loader, addr)) {
    return true;
  }
  uint64_t runtime = 0;
  if (LoaderLinkedAddressToRuntime(loader, NULL, addr, &runtime) &&
      runtime != addr && AddressInExecutableRegion(loader, runtime)) {
    return true;
  }
  return false;
}

uint64_t RISCVLookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !RISCVGuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

void RISCVGuestCallVoidFunction(RISCVInterpreter* interpreter, uint64_t fn) {
  if (interpreter == NULL || fn == 0) {
    return;
  }
  RISCVInterpreterCall(interpreter, fn);
}

static bool RISCVLifecycleCallback(void* context, LoadedDynamicLibrary* image,
                                   uint64_t function,
                                   LoaderLifecyclePhase phase) {
  (void)image;
  (void)phase;
  typedef struct {
    Loader* loader;
    RISCVInterpreter* interpreter;
  } RISCVLifecycleContext;
  RISCVLifecycleContext* ctx = context;
  if (!RISCVGuestAddressExecutable(ctx->loader, function)) {
    LoaderError("Function array entry 0x%llx is not executable\n",
                (unsigned long long)function);
    return false;
  }
  RISCVGuestCallVoidFunction(ctx->interpreter, function);
  return true;
}

static bool RunGuestLifecyclePhase(Loader* loader, RISCVInterpreter* interpreter,
                                   LoaderLifecyclePhase phase) {
  typedef struct {
    Loader* loader;
    RISCVInterpreter* interpreter;
  } RISCVLifecycleContext;
  RISCVLifecycleContext ctx = {loader, interpreter};
  return LoaderLifecycleRunPhase(loader, loader->lifecycle, phase,
                                 RISCVLifecycleCallback, &ctx);
}

bool RISCVGuestRunInitArrays(Loader* loader, RISCVInterpreter* interpreter) {
  return RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecyclePreinit) &&
         RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecycleInit);
}

bool RISCVGuestRunFiniArrays(Loader* loader, RISCVInterpreter* interpreter) {
  return RunGuestLifecyclePhase(loader, interpreter, kLoaderLifecycleFini);
}

void RISCVGuestRunProgramFini(Loader* loader, RISCVInterpreter* interpreter) {
  RISCVGuestCallVoidFunction(interpreter,
                             RISCVLookupGuestFunction(loader,
                                                      "__davecc_run_fini"));
}

bool RISCVGuestRunProgramShutdown(Loader* loader, RISCVInterpreter* interpreter) {
  if (!LoaderLifecycleExecutableFiniAlreadyDone(loader->lifecycle)) {
    uint64_t guest_fini =
        RISCVLookupGuestFunction(loader, "__davecc_run_fini");
    if (guest_fini != 0) {
      RISCVGuestCallVoidFunction(interpreter, guest_fini);
      LoaderLifecycleMarkExecutableFiniComplete(loader, loader->lifecycle);
    }
  }
  return RISCVGuestRunFiniArrays(loader, interpreter);
}

void RISCVInterpreterDestruct(RISCVInterpreter* interpreter) {
  if (interpreter->owns_stack) {
    free(interpreter->stack);
  }
}
