#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

#if defined(__DAVECC_NATIVE_LINUX__)

void* mmap(void* address, size_t length, int protection, int flags, int fd,
           off_t offset) {
#if defined(__arm__)
  if ((offset & 4095) != 0) {
    errno = EINVAL;
    return MAP_FAILED;
  }
  return (void*)syscall(SYS_mmap2, address, length, protection, flags, fd,
                        (long)(offset / 4096));
#else
  return (void*)syscall(SYS_mmap, address, length, protection, flags, fd,
                        offset);
#endif
}

int munmap(void* address, size_t length) {
  return (int)syscall(SYS_munmap, address, length, 0, 0, 0, 0);
}

int mprotect(const void* address, size_t length, int protection) {
  return (int)syscall(SYS_mprotect, address, length, protection, 0, 0, 0);
}

ssize_t pread(int fd, void* buffer, size_t size, off_t offset) {
#if defined(__arm__)
  return (ssize_t)syscall(SYS_pread64, fd, buffer, size, 0,
                          (long)offset, offset < 0 ? -1L : 0L);
#else
  return (ssize_t)syscall(SYS_pread64, fd, buffer, size, offset, 0, 0);
#endif
}

pid_t fork(void) {
  return (pid_t)syscall(SYS_clone, 17, 0, 0, 0, 0, 0);
}

pid_t waitpid(pid_t pid, int* status, int options) {
#if defined(__risc_v__) && defined(__ILP32__)
  // RV32 uses the post-2038 asm-generic syscall set, which omits wait4.
  // waitid returns the same information in siginfo_t; translate its child
  // fields to the traditional waitpid status encoding.
  uint32_t info[32] = {0};
  int id_type;
  unsigned int id;
  if (pid > 0) {
    id_type = 1;  // P_PID
    id = (unsigned int)pid;
  } else if (pid == -1) {
    id_type = 0;  // P_ALL
    id = 0;
  } else {
    id_type = 2;  // P_PGID
    id = pid == 0 ? 0 : (unsigned int)-pid;
  }
  if (syscall(SYS_waitid, id_type, id, info, options | 4, NULL, 0) < 0) {
    return -1;
  }
  pid_t result = (pid_t)info[3];
  if (result == 0) {
    return 0;
  }
  if (status != NULL) {
    int code = (int)info[2];
    int child_status = (int)info[5];
    if (code == 1) {          // CLD_EXITED
      *status = child_status << 8;
    } else if (code == 2) {   // CLD_KILLED
      *status = child_status;
    } else if (code == 3) {   // CLD_DUMPED
      *status = child_status | 0x80;
    } else if (code == 5) {   // CLD_STOPPED
      *status = (child_status << 8) | 0x7f;
    } else if (code == 6) {   // CLD_CONTINUED
      *status = 0xffff;
    } else {
      *status = 0;
    }
  }
  return result;
#else
  return (pid_t)syscall(SYS_wait4, pid, status, options, NULL, 0, 0);
#endif
}

int isatty(int fd) {
  struct {
    uint16_t rows;
    uint16_t columns;
    uint16_t xpixel;
    uint16_t ypixel;
  } window;
  if (syscall(SYS_ioctl, fd, 0x5413, &window, 0, 0, 0) == 0) return 1;
  return 0;
}

long sysconf(int name) {
  if (name == _SC_PAGESIZE) return 4096;
  errno = EINVAL;
  return -1;
}

#else

void* mmap(void* address, size_t length, int protection, int flags, int fd,
           off_t offset) {
  (void)address;
  (void)length;
  (void)protection;
  (void)flags;
  (void)fd;
  (void)offset;
  errno = ENOSYS;
  return MAP_FAILED;
}

int munmap(void* address, size_t length) {
  (void)address;
  (void)length;
  errno = ENOSYS;
  return -1;
}

int mprotect(const void* address, size_t length, int protection) {
  (void)address;
  (void)length;
  (void)protection;
  errno = ENOSYS;
  return -1;
}

ssize_t pread(int fd, void* buffer, size_t size, off_t offset) {
  (void)fd;
  (void)buffer;
  (void)size;
  (void)offset;
  errno = ENOSYS;
  return -1;
}

pid_t fork(void) {
  errno = ENOSYS;
  return -1;
}

pid_t waitpid(pid_t pid, int* status, int options) {
  (void)pid;
  (void)status;
  (void)options;
  errno = ENOSYS;
  return -1;
}

int isatty(int fd) {
  (void)fd;
  errno = ENOTTY;
  return 0;
}

long sysconf(int name) {
  if (name == _SC_PAGESIZE) return 4096;
  errno = EINVAL;
  return -1;
}

#endif
