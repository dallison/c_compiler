#include <errno.h>

long __davecc_linux_syscall_result(long result) {
  if ((unsigned long)result >= (unsigned long)-4095) {
    errno = (int)-result;
    return -1;
  }
  return result;
}
