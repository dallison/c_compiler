#ifndef davecc_sys_eventfd_h
#define davecc_sys_eventfd_h

#include <stdint.h>

#define EFD_SEMAPHORE 00000001
#define EFD_CLOEXEC 02000000
#define EFD_NONBLOCK 00004000

#ifdef __cplusplus
extern "C" {
#endif

int eventfd(unsigned int initval, int flags);

#ifdef __cplusplus
}
#endif

#endif
