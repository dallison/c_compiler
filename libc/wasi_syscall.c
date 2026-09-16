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
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syscall.h>
#include <time.h>

#include "posix_fs.h"

// WASI preview 1.  Every call answers with an error number rather than the
// -1-and-errno of a Unix syscall, and anything it has to hand back is
// written through a pointer.
typedef struct {
  const void* buf;
  unsigned buf_len;
} WasiIovec;

typedef struct {
  unsigned char tag;
  unsigned char pad[3];
  unsigned name_len;
} WasiPrestat;

typedef struct {
  unsigned long long dev;
  unsigned long long ino;
  unsigned char filetype;
  unsigned char pad[7];
  unsigned long long nlink;
  unsigned long long size;
  unsigned long long atim;
  unsigned long long mtim;
  unsigned long long ctim;
} WasiFilestat;

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
extern int __wasi_fd_prestat_get(int fd, WasiPrestat* prestat);
extern int __wasi_fd_prestat_dir_name(int fd, char* path, unsigned path_len);
extern int __wasi_path_open(int dirfd, unsigned dirflags, const char* path,
                            unsigned path_len, unsigned oflags,
                            unsigned long long fs_rights_base,
                            unsigned long long fs_rights_inheriting,
                            unsigned fdflags, int* opened_fd);
extern int __wasi_path_unlink_file(int dirfd, const char* path,
                                   unsigned path_len);
extern int __wasi_path_remove_directory(int dirfd, const char* path,
                                        unsigned path_len);
extern int __wasi_path_filestat_get(int dirfd, unsigned flags, const char* path,
                                    unsigned path_len, WasiFilestat* buf);
extern int __wasi_fd_filestat_get(int fd, WasiFilestat* buf);

#define WASI_CLOCK_REALTIME 0
#define WASI_CLOCK_MONOTONIC 1
#define WASI_CLOCK_PROCESS_CPUTIME 2

#define WASI_LOOKUP_SYMLINK_FOLLOW 1u

#define WASI_OFLAGS_CREAT 1u
#define WASI_OFLAGS_DIRECTORY 2u
#define WASI_OFLAGS_EXCL 4u
#define WASI_OFLAGS_TRUNC 8u

#define WASI_FDFLAG_APPEND 1u

#define WASI_FILETYPE_DIRECTORY 3
#define WASI_FILETYPE_REGULAR_FILE 4
#define WASI_FILETYPE_SYMBOLIC_LINK 7

#define WASI_RIGHT_FD_DATASYNC (1ull << 0)
#define WASI_RIGHT_FD_READ (1ull << 1)
#define WASI_RIGHT_FD_SEEK (1ull << 2)
#define WASI_RIGHT_FD_FDSTAT_SET_FLAGS (1ull << 3)
#define WASI_RIGHT_FD_SYNC (1ull << 4)
#define WASI_RIGHT_FD_TELL (1ull << 5)
#define WASI_RIGHT_FD_WRITE (1ull << 6)
#define WASI_RIGHT_PATH_CREATE_DIRECTORY (1ull << 9)
#define WASI_RIGHT_PATH_CREATE_FILE (1ull << 10)
#define WASI_RIGHT_PATH_OPEN (1ull << 13)
#define WASI_RIGHT_PATH_FILESTAT_GET (1ull << 18)
#define WASI_RIGHT_PATH_FILESTAT_SET_SIZE (1ull << 19)
#define WASI_RIGHT_FD_FILESTAT_GET (1ull << 21)
#define WASI_RIGHT_FD_FILESTAT_SET_SIZE (1ull << 22)
#define WASI_RIGHT_PATH_REMOVE_DIRECTORY (1ull << 25)
#define WASI_RIGHT_PATH_UNLINK_FILE (1ull << 26)

typedef struct {
  int fd;
  unsigned name_len;
  char* name;
} WasiPreopen;

static WasiPreopen g_preopens[16];
static int g_num_preopens = -1;
static char g_cwd[PATH_MAX] = "/";
static char g_relbuf[PATH_MAX];

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

static long FailPosix(int error) { return -(long)error; }

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

static void DiscoverPreopens(void) {
  if (g_num_preopens >= 0) {
    return;
  }
  g_num_preopens = 0;
  for (int fd = 3; fd < 32 && g_num_preopens < 16; fd++) {
    WasiPrestat prestat;
    if (__wasi_fd_prestat_get(fd, &prestat) != 0) {
      break;
    }
    if (prestat.tag != 0 || prestat.name_len == 0 ||
        prestat.name_len >= PATH_MAX) {
      continue;
    }
    char* name = malloc(prestat.name_len + 1);
    if (name == NULL) {
      continue;
    }
    if (__wasi_fd_prestat_dir_name(fd, name, prestat.name_len) != 0) {
      free(name);
      continue;
    }
    name[prestat.name_len] = '\0';
    g_preopens[g_num_preopens].fd = fd;
    g_preopens[g_num_preopens].name_len = prestat.name_len;
    g_preopens[g_num_preopens].name = name;
    g_num_preopens++;
    if (prestat.name_len == 1 && name[0] == '/') {
      g_cwd[0] = '/';
      g_cwd[1] = '\0';
    }
  }
}

static int JoinPath(const char* path, char* out, size_t out_size) {
  if (path == NULL || path[0] == '\0') {
    return EINVAL;
  }
  if (path[0] == '/') {
    if (strlen(path) >= out_size) {
      return ENAMETOOLONG;
    }
    strcpy(out, path);
    return 0;
  }
  size_t cwd_len = strlen(g_cwd);
  size_t path_len = strlen(path);
  int need_slash = cwd_len > 0 && g_cwd[cwd_len - 1] != '/';
  if (cwd_len + (size_t)need_slash + path_len >= out_size) {
    return ENAMETOOLONG;
  }
  memcpy(out, g_cwd, cwd_len);
  if (need_slash) {
    out[cwd_len++] = '/';
  }
  memcpy(out + cwd_len, path, path_len + 1);
  return 0;
}

static int MatchPreopen(const char* abs, int* dirfd, const char** relative) {
  int best = -1;
  unsigned best_len = 0;
  for (int i = 0; i < g_num_preopens; i++) {
    const char* name = g_preopens[i].name;
    unsigned nlen = g_preopens[i].name_len;
    if (nlen == 1 && name[0] == '/') {
      if (abs[0] != '/') {
        continue;
      }
      if (best < 0 || nlen >= best_len) {
        best = i;
        best_len = nlen;
      }
      continue;
    }
    if (strncmp(abs, name, nlen) != 0) {
      continue;
    }
    if (abs[nlen] != '\0' && abs[nlen] != '/') {
      continue;
    }
    if (nlen >= best_len) {
      best = i;
      best_len = nlen;
    }
  }
  if (best < 0) {
    // A relative name can also be opened against a preopen of "." without
    // going through the cwd, which is what `wasmtime run --dir=.` provides.
    if (abs[0] != '/') {
      for (int i = 0; i < g_num_preopens; i++) {
        if (g_preopens[i].name_len == 1 && g_preopens[i].name[0] == '.') {
          *dirfd = g_preopens[i].fd;
          *relative = abs;
          return 0;
        }
      }
    }
    return ENOENT;
  }
  *dirfd = g_preopens[best].fd;
  const char* rest = abs + best_len;
  while (*rest == '/') {
    rest++;
  }
  *relative = *rest == '\0' ? "." : rest;
  return 0;
}

static int ResolvePath(const char* path, int* dirfd, const char** relative) {
  DiscoverPreopens();
  char abs[PATH_MAX];
  if (path != NULL && path[0] != '/') {
    for (int i = 0; i < g_num_preopens; i++) {
      if (g_preopens[i].name_len == 1 && g_preopens[i].name[0] == '.') {
        *dirfd = g_preopens[i].fd;
        *relative = path;
        return 0;
      }
    }
  }
  int error = JoinPath(path, abs, sizeof(abs));
  if (error != 0) {
    return error;
  }
  error = MatchPreopen(abs, dirfd, relative);
  if (error != 0) {
    return error;
  }
  if (*relative != g_relbuf) {
    size_t length = strlen(*relative);
    if (length >= sizeof(g_relbuf)) {
      return ENAMETOOLONG;
    }
    memcpy(g_relbuf, *relative, length + 1);
    *relative = g_relbuf;
  }
  return 0;
}

static unsigned OpenOflags(int flags) {
  unsigned oflags = 0;
  if (flags & O_CREAT) {
    oflags |= WASI_OFLAGS_CREAT;
  }
  if (flags & O_EXCL) {
    oflags |= WASI_OFLAGS_EXCL;
  }
  if (flags & O_TRUNC) {
    oflags |= WASI_OFLAGS_TRUNC;
  }
  if (flags & O_DIRECTORY) {
    oflags |= WASI_OFLAGS_DIRECTORY;
  }
  return oflags;
}

static unsigned OpenFdflags(int flags) {
  return (flags & O_APPEND) ? WASI_FDFLAG_APPEND : 0;
}

static unsigned long long OpenRights(int flags) {
  unsigned long long rights = WASI_RIGHT_FD_SEEK | WASI_RIGHT_FD_TELL |
                              WASI_RIGHT_FD_FILESTAT_GET |
                              WASI_RIGHT_FD_FDSTAT_SET_FLAGS;
  int acc = flags & O_ACCMODE;
  if (acc != O_WRONLY) {
    rights |= WASI_RIGHT_FD_READ;
  }
  if (acc != O_RDONLY) {
    rights |= WASI_RIGHT_FD_WRITE | WASI_RIGHT_FD_DATASYNC | WASI_RIGHT_FD_SYNC |
              WASI_RIGHT_FD_FILESTAT_SET_SIZE;
  }
  return rights;
}

static long DoOpen(const char* path, int flags) {
  int dirfd = 0;
  const char* relative = NULL;
  int error = ResolvePath(path, &dirfd, &relative);
  if (error != 0) {
    return FailPosix(error);
  }
  int opened = -1;
  error = __wasi_path_open(dirfd, WASI_LOOKUP_SYMLINK_FOLLOW, relative,
                           (unsigned)strlen(relative), OpenOflags(flags),
                           OpenRights(flags), OpenRights(flags),
                           OpenFdflags(flags), &opened);
  return error == 0 ? opened : Fail(error);
}

static void FillWireStatus(DaveWireStatus* wire, const WasiFilestat* stat) {
  memset(wire, 0, sizeof(*wire));
  wire->device = stat->dev;
  wire->inode = stat->ino;
  wire->size = stat->size;
  wire->hard_link_count = stat->nlink;
  wire->access_time_ns = (int64_t)stat->atim;
  wire->modification_time_ns = (int64_t)stat->mtim;
  wire->status_change_time_ns = (int64_t)stat->ctim;
  uint32_t mode = 0644;
  if (stat->filetype == WASI_FILETYPE_DIRECTORY) {
    mode = S_IFDIR | 0755;
  } else if (stat->filetype == WASI_FILETYPE_SYMBOLIC_LINK) {
    mode = S_IFLNK | 0777;
  } else {
    mode = S_IFREG | 0644;
  }
  wire->mode = mode;
}

static long DoPathStat(const char* path, int follow, DaveWireStatus* wire) {
  int dirfd = 0;
  const char* relative = NULL;
  int error = ResolvePath(path, &dirfd, &relative);
  if (error != 0) {
    return FailPosix(error);
  }
  WasiFilestat stat;
  error = __wasi_path_filestat_get(dirfd, follow ? WASI_LOOKUP_SYMLINK_FOLLOW : 0,
                                   relative, (unsigned)strlen(relative), &stat);
  if (error != 0) {
    return Fail(error);
  }
  FillWireStatus(wire, &stat);
  return 0;
}

static long DoFdStat(int fd, DaveWireStatus* wire) {
  WasiFilestat stat;
  int error = __wasi_fd_filestat_get(fd, &stat);
  if (error != 0) {
    return Fail(error);
  }
  FillWireStatus(wire, &stat);
  return 0;
}

static long DoRemove(const char* path) {
  int dirfd = 0;
  const char* relative = NULL;
  int error = ResolvePath(path, &dirfd, &relative);
  if (error != 0) {
    return FailPosix(error);
  }
  unsigned length = (unsigned)strlen(relative);
  error = __wasi_path_unlink_file(dirfd, relative, length);
  if (error == 31) {
    // WASI reports EISDIR when the path names a directory, so try rmdir.
    error = __wasi_path_remove_directory(dirfd, relative, length);
  }
  return error == 0 ? 0 : Fail(error);
}

static long DoGetcwd(char* buffer, size_t size) {
  DiscoverPreopens();
  size_t length = strlen(g_cwd);
  if (size == 0 || length + 1 > size) {
    return FailPosix(EOVERFLOW);
  }
  memcpy(buffer, g_cwd, length + 1);
  return (long)length;
}

static long DoCanonical(const char* path, char* resolved, size_t size) {
  char abs[PATH_MAX];
  int error = JoinPath(path, abs, sizeof(abs));
  if (error != 0) {
    return FailPosix(error);
  }
  DaveWireStatus wire;
  long result = DoPathStat(abs, 1, &wire);
  if (result < 0) {
    return result;
  }
  size_t length = strlen(abs);
  if (length + 1 > size) {
    return FailPosix(ENAMETOOLONG);
  }
  memcpy(resolved, abs, length + 1);
  return 0;
}

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
    case SYS_OPEN: {
      const char* path = va_arg(args, const char*);
      int flags = va_arg(args, int);
      (void)va_arg(args, int);  // mode, unused: WASI has no unix permissions.
      result = DoOpen(path, flags);
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
    case SYS_FS_STATUS: {
      const char* path = (const char*)va_arg(args, intptr_t);
      int follow = (int)va_arg(args, intptr_t);
      DaveWireStatus* wire = (DaveWireStatus*)va_arg(args, intptr_t);
      result = DoPathStat(path, follow, wire);
      break;
    }
    case SYS_FS_DESCRIPTOR_STATUS: {
      int fd = (int)va_arg(args, intptr_t);
      DaveWireStatus* wire = (DaveWireStatus*)va_arg(args, intptr_t);
      result = DoFdStat(fd, wire);
      break;
    }
    case SYS_FS_REMOVE: {
      const char* path = (const char*)va_arg(args, intptr_t);
      result = DoRemove(path);
      break;
    }
    case SYS_FS_CURRENT_PATH: {
      char* buffer = (char*)va_arg(args, intptr_t);
      size_t size = (size_t)va_arg(args, intptr_t);
      result = DoGetcwd(buffer, size);
      break;
    }
    case SYS_FS_CANONICAL: {
      const char* path = (const char*)va_arg(args, intptr_t);
      char* resolved = (char*)va_arg(args, intptr_t);
      size_t size = (size_t)va_arg(args, intptr_t);
      result = DoCanonical(path, resolved, size);
      break;
    }
    default:
      result = -ENOSYS;
      break;
  }
  va_end(args);
  return result;
}

#endif /* __wasm32__ */
