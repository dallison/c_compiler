#include <syscall.h>

#if defined(__p_code__)
int close(int fd) {
  (void)fd;
  return 0;
}
#else
int close(int fd) {
  return syscall(SYS_CLOSE, fd);
}
#endif
