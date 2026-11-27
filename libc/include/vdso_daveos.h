#ifndef davecc_vdso_daveos_h
#define davecc_vdso_daveos_h

#if defined(__DAVECC_DAVEOS__)

#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 0 on success, negative errno from vDSO, or >0 if unavailable. */
int __daveos_vdso_clock_gettime(clockid_t clock_id, struct timespec* tp);
int __daveos_vdso_gettimeofday(struct timeval* tv, void* tz);
int __daveos_vdso_clock_getres(clockid_t clock_id, struct timespec* res);
int __daveos_vdso_getcpu(unsigned* cpu, unsigned* node);

/*
 * Optional host (kernel) clock backends. Libc provides weak stubs that
 * return >0 (unavailable); the DaveOS kernel overrides with strong defs
 * so the shared libcdaveos.a never issues guest SVCs from EL1.
 * Same return convention as __daveos_vdso_*: 0 ok, <0 -errno, >0 skip.
 */
int __daveos_host_clock_gettime(clockid_t clock_id, struct timespec* tp);
int __daveos_host_gettimeofday(struct timeval* tv, void* tz);
int __daveos_host_clock_getres(clockid_t clock_id, struct timespec* res);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_DAVEOS__ */

#endif
