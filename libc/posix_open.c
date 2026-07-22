#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <syscall.h>

// The open function can take an extra arg for the open mode
// if O_CREAT is in the flags.
#if defined(__p_code__)
int open(const char* filename, int flags, ...) {
  (void)filename;
  (void)flags;
  return -1;
}
#else
int open(const char* filename, int flags, ...) {
  va_list ap;
  va_start(ap, flags);
  int mode = 0;
  if (flags & O_CREAT) {
    mode = va_arg(ap, int);
  }
  va_end(ap);
  return syscall(SYS_OPEN, filename, flags, mode);
}
#endif
