//
//  wasi_syscall.c
//  c_compiler
//
//  The bottom of libc on wasm32, where a syscall becomes a call into the
//  host.
//
//  Wasm has no trap instruction to carry a syscall on, and no register
//  convention to pass one in: a call out of the module is an ordinary
//  function call to an import, and the host names each one separately.  So
//  rather than one entry point taking a number, there is an import per WASI
//  call, and this file is the switch that turns the number the rest of libc
//  speaks into the right one.
//
//  The imports are declared by name.  A function whose name begins with
//  '__wasi_' is not defined anywhere in the program; the linker turns it
//  into an import of the rest of the name from 'wasi_snapshot_preview1'.
//

// Every target's libc archive is built from all of libc, so this has to be
// nothing at all anywhere else: its 'syscall' would otherwise stand against
// the one that speaks to a real kernel.
#if defined(__wasm32__)

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>

// WASI preview 1.  Every call answers with an error number rather than the
// -1-and-errno of a Unix syscall, and anything it has to hand back is
// written through a pointer.
typedef struct {
  const void* buf;
  unsigned buf_len;
} WasiIovec;

extern int __wasi_fd_write(int fd, const WasiIovec* iovs, unsigned iovs_len,
                           unsigned* written);
extern int __wasi_fd_read(int fd, const WasiIovec* iovs, unsigned iovs_len,
                          unsigned* read);
extern int __wasi_fd_close(int fd);
extern int __wasi_fd_seek(int fd, long long offset, int whence,
                          unsigned long long* position);
extern int __wasi_clock_time_get(int clock_id, unsigned long long precision,
                                 unsigned long long* time);
extern int __wasi_random_get(void* buf, unsigned buf_len);
extern void __wasi_proc_exit(int status);

#define WASI_CLOCK_REALTIME 0
#define WASI_CLOCK_MONOTONIC 1
#define WASI_CLOCK_PROCESS_CPUTIME 2

// WASI numbers its errors itself, so what comes back has to be turned into
// the numbering the rest of libc uses.  Anything not named here is a error
// no caller in libc distinguishes, and becomes EUNKNOWN rather than being
// silently reported as some unrelated failure.
static int TranslateErrno(int error) {
  switch (error) {
    case 2: return EACCES;
    case 6: return EAGAIN;
    case 8: return EBADF;
    case 10: return EBUSY;
    case 16: return EDEADLK;
    case 18: return EDOM;
    case 20: return EEXIST;
    case 22: return EFBIG;
    case 25: return EILSEQ;
    case 27: return EINTR;
    case 28: return EINVAL;
    case 29: return EIO;
    case 31: return EISDIR;
    case 32: return ELOOP;
    case 33: return EMFILE;
    case 37: return ENAMETOOLONG;
    case 41: return ENFILE;
    case 43: return ENODEV;
    case 44: return ENOENT;
    case 45: return ENOEXEC;
    case 48: return ENOMEM;
    case 51: return ENOSPC;
    case 52: return ENOSYS;
    case 54: return ENOTDIR;
    case 55: return ENOTEMPTY;
    case 58: return ENOTSUP;
    case 59: return ENOTTY;
    case 61: return EOVERFLOW;
    case 63: return EPERM;
    case 68: return ERANGE;
    case 69: return EROFS;
    case 70: return ESPIPE;
    case 71: return ESRCH;
    case 75: return EXDEV;
    default: return EUNKNOWN;
  }
}

// A syscall reports failure the way Linux does, as the negated error, which
// is what the wrappers above this expect to see.
static long Fail(int error) { return -(long)TranslateErrno(error); }

static long Transfer(int (*call)(int, const WasiIovec*, unsigned, unsigned*),
                     int fd, const void* buffer, size_t length) {
  WasiIovec iov;
  unsigned moved = 0;
  iov.buf = buffer;
  iov.buf_len = (unsigned)length;
  int error = call(fd, &iov, 1, &moved);
  return error == 0 ? (long)moved : Fail(error);
}

// fd_read wants the same shape as fd_write but will not accept a pointer to
// const, so it gets there through a cast rather than a second copy of the
// code above.
typedef int (*WasiTransfer)(int, const WasiIovec*, unsigned, unsigned*);

long syscall(int number, ...) {
  va_list args;
  va_start(args, number);
  long result;
  switch (number) {
    case SYS_WRITE: {
      int fd = va_arg(args, int);
      const void* buffer = va_arg(args, const void*);
      size_t length = va_arg(args, size_t);
      result = Transfer(__wasi_fd_write, fd, buffer, length);
      break;
    }
    case SYS_READ: {
      int fd = va_arg(args, int);
      void* buffer = va_arg(args, void*);
      size_t length = va_arg(args, size_t);
      result = Transfer((WasiTransfer)__wasi_fd_read, fd, buffer, length);
      break;
    }
    case SYS_CLOSE: {
      int fd = va_arg(args, int);
      int error = __wasi_fd_close(fd);
      result = error == 0 ? 0 : Fail(error);
      break;
    }
    case SYS_LSEEK: {
      int fd = va_arg(args, int);
      long offset = va_arg(args, long);
      int whence = va_arg(args, int);
      unsigned long long position = 0;
      // SEEK_SET, SEEK_CUR and SEEK_END are 0, 1 and 2 in both numberings.
      int error = __wasi_fd_seek(fd, offset, whence, &position);
      result = error == 0 ? (long)position : Fail(error);
      break;
    }
    case SYS_TIME: {
      unsigned long long now = 0;
      int error = __wasi_clock_time_get(WASI_CLOCK_REALTIME, 1000000000ULL,
                                        &now);
      result = error == 0 ? (long)(now / 1000000000ULL) : Fail(error);
      break;
    }
    case SYS_CLOCK: {
      unsigned long long now = 0;
      int error =
          __wasi_clock_time_get(WASI_CLOCK_PROCESS_CPUTIME, 1000ULL, &now);
      if (error != 0) {
        // A host that will not measure processor time still has to answer
        // clock(), and the wall clock is the closest thing it has.
        error = __wasi_clock_time_get(WASI_CLOCK_MONOTONIC, 1000ULL, &now);
      }
      result = error == 0
                   ? (long)(now / (1000000000ULL / CLOCKS_PER_SEC))
                   : Fail(error);
      break;
    }
    case SYS_RANDOM_BYTES: {
      void* buffer = va_arg(args, void*);
      size_t length = va_arg(args, size_t);
      int error = __wasi_random_get(buffer, (unsigned)length);
      result = error == 0 ? (long)length : Fail(error);
      break;
    }
    case SYS_EXIT:
    case SYS_EXIT_CLEAN: {
      int status = va_arg(args, int);
      __wasi_proc_exit(status);
      result = 0;
      break;
    }
    case SYS_ABORT:
      __wasi_proc_exit(134);  // What a shell reports for a process killed by
      result = 0;             // SIGABRT, which is the nearest thing to it.
      break;
    default:
      result = -ENOSYS;
      break;
  }
  va_end(args);
  return result;
}

#endif /* __wasm32__ */
