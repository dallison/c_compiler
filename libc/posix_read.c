#include <stddef.h>
#include <syscall.h>

#if defined(__p_code__)
int read(int fd, char* buffer, size_t len) {
  (void)fd;
  (void)buffer;
  (void)len;
  return asm(
      "ldw r0, [ap, #16]\n"
      "ldx r1, [ap, #20]\n"
      "ldx r2, [ap, #28]\n"
      "esc #3");
}
#else
int read(int fd, char* buffer, size_t len) {
  return syscall(SYS_READ, fd, buffer, len);
}
#endif
