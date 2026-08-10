#include <fcntl.h>
#include <syscall.h>

long lseek(int fd, fpos_t pos, int whence) {
  return syscall(SYS_LSEEK, fd, pos, whence);
}
