#if defined(__DAVECC_DAVEOS__)

#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <time.h>

typedef int (*vdso_clock_gettime_t)(int, struct timespec*);
typedef int (*vdso_gettimeofday_t)(struct timeval*, void*);
typedef int (*vdso_clock_getres_t)(int, struct timespec*);
typedef int (*vdso_getcpu_t)(unsigned*, unsigned*, void*);

static vdso_clock_gettime_t g_clock_gettime;
static vdso_gettimeofday_t g_gettimeofday;
static vdso_clock_getres_t g_clock_getres;
static vdso_getcpu_t g_getcpu;
static int g_resolved;

static uint64_t dyn_val(const Elf64_Dyn* dyn, int64_t tag) {
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == tag) {
      return d->d_un.d_val;
    }
  }
  return 0;
}

static void* vdso_dlsym(uintptr_t base, const char* name) {
  const Elf64_Ehdr* eh = (const Elf64_Ehdr*)base;
  if (eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E') {
    return NULL;
  }
  const Elf64_Phdr* ph = (const Elf64_Phdr*)(base + eh->e_phoff);
  const Elf64_Dyn* dyn = NULL;
  for (Elf64_Half i = 0; i < eh->e_phnum; i++) {
    if (ph[i].p_type == PT_DYNAMIC) {
      dyn = (const Elf64_Dyn*)(base + ph[i].p_vaddr);
      break;
    }
  }
  if (dyn == NULL) {
    return NULL;
  }
  uint64_t symtab = dyn_val(dyn, DT_SYMTAB);
  uint64_t strtab = dyn_val(dyn, DT_STRTAB);
  uint64_t syment = dyn_val(dyn, DT_SYMENT);
  if (syment == 0) {
    syment = sizeof(Elf64_Sym);
  }
  if (symtab == 0 || strtab == 0) {
    return NULL;
  }
  const char* strs = (const char*)(base + strtab);
  const Elf64_Sym* syms = (const Elf64_Sym*)(base + symtab);
  for (size_t i = 1; i < 256; i++) {
    const Elf64_Sym* sym = (const Elf64_Sym*)((const char*)syms + i * syment);
    if (sym->st_name == 0) {
      continue;
    }
    if (strcmp(strs + sym->st_name, name) == 0) {
      return (void*)(base + sym->st_value);
    }
  }
  return NULL;
}

static void vdso_resolve(void) {
  if (g_resolved) {
    return;
  }
  g_resolved = 1;
  uintptr_t base = (uintptr_t)getauxval(AT_SYSINFO_EHDR);
  if (base == 0) {
    return;
  }
  g_clock_gettime =
      (vdso_clock_gettime_t)vdso_dlsym(base, "__kernel_clock_gettime");
  g_gettimeofday =
      (vdso_gettimeofday_t)vdso_dlsym(base, "__kernel_gettimeofday");
  g_clock_getres =
      (vdso_clock_getres_t)vdso_dlsym(base, "__kernel_clock_getres");
  g_getcpu = (vdso_getcpu_t)vdso_dlsym(base, "__kernel_getcpu");
}

int __daveos_vdso_clock_gettime(clockid_t clock_id, struct timespec* tp) {
  vdso_resolve();
  if (g_clock_gettime == NULL) {
    return 1;
  }
  return g_clock_gettime((int)clock_id, tp);
}

int __daveos_vdso_gettimeofday(struct timeval* tv, void* tz) {
  vdso_resolve();
  if (g_gettimeofday == NULL) {
    return 1;
  }
  return g_gettimeofday(tv, tz);
}

int __daveos_vdso_clock_getres(clockid_t clock_id, struct timespec* res) {
  vdso_resolve();
  if (g_clock_getres == NULL) {
    return 1;
  }
  return g_clock_getres((int)clock_id, res);
}

int __daveos_vdso_getcpu(unsigned* cpu, unsigned* node) {
  vdso_resolve();
  if (g_getcpu == NULL) {
    return 1;
  }
  return g_getcpu(cpu, node, NULL);
}

__attribute__((weak)) int __daveos_host_clock_gettime(clockid_t clock_id,
                                                      struct timespec* tp) {
  (void)clock_id;
  (void)tp;
  return 1;
}

__attribute__((weak)) int __daveos_host_gettimeofday(struct timeval* tv,
                                                     void* tz) {
  (void)tv;
  (void)tz;
  return 1;
}

__attribute__((weak)) int __daveos_host_clock_getres(clockid_t clock_id,
                                                     struct timespec* res) {
  (void)clock_id;
  (void)res;
  return 1;
}

#endif /* __DAVECC_DAVEOS__ */
