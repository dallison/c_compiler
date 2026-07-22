#include <fcntl.h>
#include <syscall.h>

#if defined(__p_code__)
long lseek(int fd, fpos_t pos, int whence) {
  (void)fd;
  (void)pos;
  (void)whence;
  return -1;
}
#else
long lseek(int fd, fpos_t pos, int whence) {
  return syscall(SYS_LSEEK, fd, pos, whence);
}
#endif
