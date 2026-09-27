//
//  sysctl.h
//  libc
//
//  Minimal <sys/sysctl.h> so Darwin sources can call sysctlbyname without
//  pulling in the SDK header stack (audit attributes, uid_t, int64_t).
//

#ifndef sys_sysctl_h
#define sys_sysctl_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int sysctlbyname(const char* name, void* oldp, size_t* oldlenp, void* newp,
                 size_t newlen);

#ifdef __cplusplus
}
#endif

#endif
