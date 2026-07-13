//
//  x86_64_native.c
//  x86_64_interpreter
//

#include "x86_64_native.h"
#include "x86_64_process.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#endif

#if defined(__x86_64__)
static Region* FindRegionContaining(Loader* loader, uint64_t address) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    uint64_t base = (uint64_t)(uintptr_t)region->address;
    if (address >= base && address < base + (uint64_t)region->length) {
      return region;
    }
  }
  return NULL;
}

static void FlushInstructionCache(void* start, size_t length) {
#if defined(__APPLE__)
  sys_icache_invalidate(start, length);
#else
  __builtin___clear_cache((char*)start, (char*)start + length);
#endif
}

static bool MakeRegionsExecutable(Loader* loader) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL ||
        (region->segment->flags & PF(x)) == 0 ||
        (region->segment->flags & PF(w)) != 0) {
      continue;
    }
    if (mprotect(region->address, (size_t)region->length,
                 PROT_READ | PROT_EXEC) != 0) {
      fprintf(stderr, "Native: mprotect failed: %s\n", strerror(errno));
      return false;
    }
    FlushInstructionCache(region->address, (size_t)region->length);
  }
  return true;
}

static bool MakeEntryExecutable(Loader* loader, uint64_t entry_address) {
  Region* region = FindRegionContaining(loader, entry_address);
  if (region == NULL || region->segment == NULL) {
    fprintf(stderr, "Native: entry 0x%llx is not in a mapped segment\n",
            (unsigned long long)entry_address);
    return false;
  }

  if ((region->segment->flags & PF(x)) == 0) {
    fprintf(stderr, "Native: entry region is not executable\n");
    return false;
  }

  if (mprotect(region->address, (size_t)region->length,
               PROT_READ | PROT_EXEC) != 0) {
    fprintf(stderr, "Native: mprotect failed: %s\n", strerror(errno));
    return false;
  }

  FlushInstructionCache(region->address, (size_t)region->length);
  return true;
}

static int CallGuestEntry(Loader* loader, uint64_t entry, int argc,
                          char** argv) {
  if (!X86_64GuestRunInitArrays(loader, NULL)) {
    return 1;
  }
  typedef int (*GuestMainFn)(int, char**);
  GuestMainFn fn = (GuestMainFn)(uintptr_t)entry;
  int result = fn(argc, argv);
  if (!X86_64GuestRunProgramShutdown(loader, NULL)) {
    return 1;
  }
  return result;
}
#endif

bool X86_64NativeNeedsInterpreter(const Loader* loader) {
#if defined(__APPLE__)
  return loader != NULL && !loader->is_static;
#else
  (void)loader;
  return false;
#endif
}

int X86_64NativeRun(Loader* loader, uint64_t entry_address, int argc,
                    char** argv) {
#if !defined(__x86_64__)
  fprintf(stderr, "Native mode requires an x86_64 host CPU\n");
  (void)loader;
  (void)entry_address;
  (void)argc;
  (void)argv;
  return 1;
#else
  if (!loader->is_static && !MakeRegionsExecutable(loader)) {
    return 1;
  }
  if (!MakeEntryExecutable(loader, entry_address)) {
    return 1;
  }

  return CallGuestEntry(loader, entry_address, argc, argv);
#endif
}
