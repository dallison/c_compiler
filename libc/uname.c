#include <errno.h>
#include <sys/utsname.h>

#if !defined(__DAVECC_NATIVE_LINUX__) && !defined(__DAVECC_NATIVE_DARWIN__)

#include <syscall.h>

int uname(struct utsname* name) {
  long result = syscall(SYS_UNAME, name);
  if (result >= 0) {
    return 0;
  }
  errno = (int)-result;
  return -1;
}

#endif
