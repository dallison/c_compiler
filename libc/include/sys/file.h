#ifndef davecc_sys_file_h
#define davecc_sys_file_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <sys/file.h>
#else

#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

int flock(int fd, int operation);

#ifdef __cplusplus
}
#endif

#endif
#endif
