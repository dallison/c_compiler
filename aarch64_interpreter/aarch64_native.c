//
//  aarch64_native.c
//  aarch64_interpreter
//

#include "aarch64_native.h"

#include "aarch64_runtime.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#endif

#define AARCH64_BTI_C 0xD503241F

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

static bool PrepareBtiBeforeEntry(void* entry, void* region_start,
                                  uint64_t* call_entry) {
  if ((char*)entry - (char*)region_start < 4) {
    return false;
  }
  uint32_t* bti_site = (uint32_t*)((char*)entry - 4);
  if (*bti_site != 0 && *bti_site != AARCH64_BTI_C) {
    return false;
  }
  *bti_site = AARCH64_BTI_C;
  *call_entry = (uint64_t)(uintptr_t)bti_site;
  return true;
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

static bool MakeEntryExecutable(Loader* loader, uint64_t entry_address,
                                uint64_t* runtime_entry) {
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

  void* entry = (void*)(uintptr_t)entry_address;
  if (loader->is_static) {
    if (mprotect(region->address, (size_t)region->length, PROT_READ | PROT_WRITE) !=
        0) {
      fprintf(stderr, "Native: mprotect(RW) failed: %s\n", strerror(errno));
      return false;
    }
    if (!PrepareBtiBeforeEntry(entry, region->address, runtime_entry)) {
      fprintf(stderr,
              "Native: no padding word before entry for BTI landing pad\n");
      return false;
    }
  } else {
    *runtime_entry = entry_address;
  }

  if (mprotect(region->address, (size_t)region->length,
               PROT_READ | PROT_EXEC) != 0) {
    fprintf(stderr, "Native: mprotect failed: %s\n", strerror(errno));
    return false;
  }

  FlushInstructionCache(region->address, (size_t)region->length);
  return true;
}

#if defined(__aarch64__)
__attribute__((target("branch-protection=none")))
static int CallGuestEntry(Loader* loader, uint64_t entry, int argc,
                          char** argv) {
  if (!AARCH64GuestRunInitArrays(loader, NULL)) {
    return 1;
  }
  typedef int (*GuestMainFn)(int, char**);
  GuestMainFn fn = (GuestMainFn)(uintptr_t)entry;
  int result = fn(argc, argv);
  if (!AARCH64GuestRunProgramShutdown(loader, NULL)) {
    return 1;
  }
  return result;
}
#endif

bool AARCH64NativeNeedsInterpreter(const Loader* loader) {
#if defined(__APPLE__)
  return loader != NULL && !loader->is_static;
#else
  (void)loader;
  return false;
#endif
}

int AARCH64NativeRun(Loader* loader, uint64_t entry_address, int argc,
                     char** argv) {
#if !defined(__aarch64__)
  fprintf(stderr, "Native mode requires an AArch64 host CPU\n");
  (void)loader;
  (void)entry_address;
  (void)argc;
  (void)argv;
  return 1;
#else
  uint64_t runtime_entry = entry_address;
  if (!loader->is_static && !MakeRegionsExecutable(loader)) {
    return 1;
  }
  if (!MakeEntryExecutable(loader, entry_address, &runtime_entry)) {
    return 1;
  }

  return CallGuestEntry(loader, runtime_entry, argc, argv);
#endif
}
