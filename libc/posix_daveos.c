/*
 * DaveOS libc wrappers: real svc calls for fork/eventfd/epoll/… .
 * Built only when compiling with -target aarch64-unknown-daveos-davecc.
 */

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

#if defined(__DAVECC_DAVEOS__)

#include <davecc_daveos_syscalls.h>

/* DaveOS returns -1 on error without a negative errno code in x0. */
static int Fail(long result) {
  if (result >= 0) return (int)result;
  errno = EIO;
  return -1;
}

static long FailLong(long result) {
  if (result >= 0) return result;
  errno = EIO;
  return -1;
}

/* POSIX sigset bit (sig-1) <-> DaveOS mask bit (sig). */
static unsigned SigsetToDave(sigset_t set) {
  unsigned mask = 0;
  for (int sig = 1; sig < 32; sig++) {
    if ((set & ((sigset_t)1 << (sig - 1))) != 0) {
      mask |= 1u << sig;
    }
  }
  return mask;
}

static sigset_t DaveToSigset(unsigned mask) {
  sigset_t set = 0;
  for (int sig = 1; sig < 32; sig++) {
    if ((mask & (1u << sig)) != 0) {
      set |= (sigset_t)1 << (sig - 1);
    }
  }
  return set;
}

pid_t fork(void) {
  return (pid_t)FailLong(syscall(DAVEOS_SYS_FORK));
}

pid_t setsid(void) {
  return (pid_t)FailLong(syscall(DAVEOS_SYS_SETSID));
}

pid_t waitpid(pid_t pid, int* status, int options) {
  (void)options; /* WNOHANG / WUNTRACED not implemented yet */
  long result = syscall(DAVEOS_SYS_WAITPID, (long)pid, status);
  return (pid_t)FailLong(result);
}

int kill(pid_t pid, int signal_number) {
  return Fail(syscall(DAVEOS_SYS_KILL, (long)pid, signal_number));
}

int sigaction(int signal_number, const struct sigaction* action,
              struct sigaction* old_action) {
  struct daveos_sigaction neu;
  struct daveos_sigaction old;
  memset(&neu, 0, sizeof(neu));
  memset(&old, 0, sizeof(old));
  if (action != NULL) {
    neu.sa_handler = (unsigned long long)(uintptr_t)action->sa_handler;
    neu.sa_mask = SigsetToDave(action->sa_mask);
    neu.sa_flags = (unsigned)action->sa_flags;
  }
  long result = syscall(DAVEOS_SYS_SIGACTION, signal_number,
                        action != NULL ? &neu : (void*)0,
                        old_action != NULL ? &old : (void*)0);
  if (result < 0) {
    errno = EIO;
    return -1;
  }
  if (old_action != NULL) {
    memset(old_action, 0, sizeof(*old_action));
    old_action->sa_handler = (void (*)(int))(uintptr_t)old.sa_handler;
    old_action->sa_mask = DaveToSigset(old.sa_mask);
    old_action->sa_flags = (int)old.sa_flags;
  }
  return 0;
}

int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) {
  unsigned neu = set != NULL ? SigsetToDave(*set) : 0;
  unsigned old = 0;
  long result = syscall(DAVEOS_SYS_SIGPROCMASK, how,
                        set != NULL ? &neu : (void*)0,
                        oldset != NULL ? &old : (void*)0);
  if (result < 0) {
    errno = EIO;
    return -1;
  }
  if (oldset != NULL) {
    *oldset = DaveToSigset(old);
  }
  return 0;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
  return pthread_sigmask(how, set, oldset);
}

int eventfd(unsigned int initval, int flags) {
  return Fail(syscall(DAVEOS_SYS_EVENTFD, (long)initval, flags));
}

int timerfd_create(int clockid, int flags) {
  return Fail(syscall(DAVEOS_SYS_TIMERFD_CREATE, clockid, flags));
}

int timerfd_settime(int fd, int flags, const struct itimerspec* new_value,
                    struct itimerspec* old_value) {
  return Fail(syscall(DAVEOS_SYS_TIMERFD_SETTIME, fd, flags, new_value,
                      old_value));
}

int timerfd_gettime(int fd, struct itimerspec* curr_value) {
  return Fail(syscall(DAVEOS_SYS_TIMERFD_GETTIME, fd, curr_value));
}

int gettimeofday(struct timeval* tv, void* tz) {
  if (tv == NULL) {
    errno = EFAULT;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_GETTIMEOFDAY, tv, tz));
}

int epoll_create(int size) {
  (void)size;
  return epoll_create1(0);
}

int epoll_create1(int flags) {
  return Fail(syscall(DAVEOS_SYS_EPOLL_CREATE1, flags));
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {
  return Fail(syscall(DAVEOS_SYS_EPOLL_CTL, epfd, op, fd, event));
}

int epoll_wait(int epfd, struct epoll_event* events, int maxevents,
               int timeout) {
  return Fail(syscall(DAVEOS_SYS_EPOLL_WAIT, epfd, events, maxevents, timeout));
}

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
           struct timeval* timeout) {
  if (nfds < 0 || nfds > DAVEOS_FD_SETSIZE) {
    errno = EINVAL;
    return -1;
  }
  struct daveos_fd_set r = {0}, w = {0}, e = {0};
  if (readfds != NULL) {
    r.mask = (unsigned long long)readfds->fds_bits[0];
  }
  if (writefds != NULL) {
    w.mask = (unsigned long long)writefds->fds_bits[0];
  }
  if (exceptfds != NULL) {
    e.mask = (unsigned long long)exceptfds->fds_bits[0];
  }
  long result =
      syscall(DAVEOS_SYS_SELECT, nfds, readfds != NULL ? &r : (void*)0,
              writefds != NULL ? &w : (void*)0,
              exceptfds != NULL ? &e : (void*)0, timeout);
  if (result < 0) {
    errno = EIO;
    return -1;
  }
  if (readfds != NULL) {
    FD_ZERO(readfds);
    readfds->fds_bits[0] = (unsigned long)r.mask;
  }
  if (writefds != NULL) {
    FD_ZERO(writefds);
    writefds->fds_bits[0] = (unsigned long)w.mask;
  }
  if (exceptfds != NULL) {
    FD_ZERO(exceptfds);
    exceptfds->fds_bits[0] = (unsigned long)e.mask;
  }
  return (int)result;
}

int execve(const char* path, char** argv, char** envp) {
  if (path == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_EXEC, path, argv, envp));
}

int execv(const char* path, char** argv) {
  return execve(path, argv, environ);
}

int daveos_exec(const char* path) {
  char* argv[2];
  if (path == NULL) {
    errno = EINVAL;
    return -1;
  }
  argv[0] = (char*)path;
  argv[1] = NULL;
  return execve(path, argv, environ);
}

int execl(const char* path, ...) {
  char* argv[64];
  va_list ap;
  size_t n = 0;
  if (path == NULL) {
    errno = EINVAL;
    return -1;
  }
  va_start(ap, path);
  while (n < 63) {
    char* a = va_arg(ap, char*);
    argv[n++] = a;
    if (a == NULL) {
      break;
    }
  }
  va_end(ap);
  if (n == 63 && argv[62] != NULL) {
    errno = E2BIG;
    return -1;
  }
  return execve(path, argv, environ);
}

int execle(const char* path, ...) {
  char* argv[64];
  char** envp;
  va_list ap;
  size_t n = 0;
  if (path == NULL) {
    errno = EINVAL;
    return -1;
  }
  va_start(ap, path);
  while (n < 63) {
    char* a = va_arg(ap, char*);
    argv[n++] = a;
    if (a == NULL) {
      break;
    }
  }
  envp = va_arg(ap, char**);
  va_end(ap);
  if (n == 63 && argv[62] != NULL) {
    errno = E2BIG;
    return -1;
  }
  return execve(path, argv, envp);
}

int execvp(const char* file, char** argv) {
  char scratch[512];
  const char* path;
  if (file == NULL || argv == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (strchr(file, '/') != NULL) {
    return execve(file, argv, environ);
  }
  path = getenv("PATH");
  if (path == NULL || path[0] == '\0') {
    path = "/";
  }
  while (*path) {
    const char* colon = strchr(path, ':');
    size_t len = colon ? (size_t)(colon - path) : strlen(path);
    size_t flen = strlen(file);
    if (len + 1 + flen + 1 > sizeof(scratch)) {
      errno = ENAMETOOLONG;
      return -1;
    }
    if (len == 0) {
      /* empty PATH component => cwd */
      memcpy(scratch, file, flen + 1);
    } else {
      memcpy(scratch, path, len);
      scratch[len] = '/';
      memcpy(scratch + len + 1, file, flen + 1);
    }
    execve(scratch, argv, environ);
    /* DaveOS maps most failures to EIO; keep searching PATH. */
    if (colon == NULL) {
      break;
    }
    path = colon + 1;
  }
  errno = ENOENT;
  return -1;
}

int execlp(const char* file, ...) {
  char* argv[64];
  va_list ap;
  size_t n = 0;
  if (file == NULL) {
    errno = EINVAL;
    return -1;
  }
  va_start(ap, file);
  while (n < 63) {
    char* a = va_arg(ap, char*);
    argv[n++] = a;
    if (a == NULL) {
      break;
    }
  }
  va_end(ap);
  if (n == 63 && argv[62] != NULL) {
    errno = E2BIG;
    return -1;
  }
  return execvp(file, argv);
}

int init_module(const char* path) {
  if (path == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_INIT_MODULE, path));
}

int delete_module(const char* name) {
  if (name == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_DELETE_MODULE, name));
}

int daveos_netif(int op, struct daveos_netif* nif) {
  if (nif == NULL || (op != DAVEOS_NETIF_GET && op != DAVEOS_NETIF_SET)) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_NETIF, op, nif));
}

int daveos_ping(unsigned addr, unsigned timeout_ms) {
  return Fail(syscall(DAVEOS_SYS_PING, (long)addr, (long)timeout_ms));
}

int sendfd(int pipe_fd, int fd) {
  return Fail(syscall(DAVEOS_SYS_SENDFD, pipe_fd, fd));
}

int recvfd(int pipe_fd) {
  return Fail(syscall(DAVEOS_SYS_RECVFD, pipe_fd));
}

int daveos_getprocs(struct daveos_proc_info* buf, int max) {
  if (buf == NULL || max <= 0) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_GETPROCS, buf, max));
}

int daveos_getmounts(struct daveos_mount_info* buf, int max) {
  if (buf == NULL || max <= 0) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_GETMOUNTS, buf, max));
}

int mount(const char* source, const char* target, const char* filesystemtype,
          unsigned long mountflags, const void* data) {
  if (source == NULL || target == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_MOUNT, source, target, filesystemtype,
                      mountflags, data));
}

int umount2(const char* target, int flags) {
  if (target == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_UMOUNT, target, flags));
}

void* sbrk(long increment) {
  long cur = syscall(DAVEOS_SYS_BRK, 0);
  if (cur < 0) {
    errno = ENOMEM;
    return (void*)-1;
  }
  if (increment == 0) {
    return (void*)(uintptr_t)cur;
  }
  long neu =
      syscall(DAVEOS_SYS_BRK, (long)((uintptr_t)cur + (uintptr_t)increment));
  if (neu < 0) {
    errno = ENOMEM;
    return (void*)-1;
  }
  return (void*)(uintptr_t)cur;
}

/* Register ABI: x0..x5 = addr,len,prot,flags,fd,offset (matches DaveOS svc). */
void* mmap(void* address, size_t length, int protection, int flags, int fd,
           off_t offset) {
  long result = syscall(SYS_MMAP, address, length, protection, flags, fd,
                        (long)offset);
  if (result < 0) {
    errno = EIO;
    return MAP_FAILED;
  }
  return (void*)(uintptr_t)result;
}

int munmap(void* address, size_t length) {
  return Fail(syscall(SYS_MUNMAP, address, length));
}

int prlimit(pid_t pid, int resource, const struct rlimit* new_limit,
            struct rlimit* old_limit) {
  struct daveos_rlimit neu;
  struct daveos_rlimit old;
  if (new_limit != NULL) {
    neu.soft = (unsigned long long)new_limit->rlim_cur;
    neu.hard = (unsigned long long)new_limit->rlim_max;
  }
  long result =
      syscall(DAVEOS_SYS_PRLIMIT64, (long)pid, resource,
              new_limit != NULL ? &neu : (void*)0,
              old_limit != NULL ? &old : (void*)0);
  if (result < 0) {
    errno = EINVAL;
    return -1;
  }
  if (old_limit != NULL) {
    old_limit->rlim_cur = (rlim_t)old.soft;
    old_limit->rlim_max = (rlim_t)old.hard;
  }
  return 0;
}

int getrlimit(int resource, struct rlimit* limit) {
  if (limit == NULL) {
    errno = EFAULT;
    return -1;
  }
  return prlimit(0, resource, NULL, limit);
}

int setrlimit(int resource, const struct rlimit* limit) {
  if (limit == NULL) {
    errno = EFAULT;
    return -1;
  }
  return prlimit(0, resource, limit, NULL);
}

int daveos_syslimit(int which, unsigned long long* new_limit,
                    unsigned long long* old_limit) {
  return Fail(syscall(DAVEOS_SYS_SYSLIMIT, which, new_limit, old_limit));
}

static char dl_error_buf[64];

void* dlopen(const char* path, int mode) {
  if (path == NULL) {
    errno = EINVAL;
    return NULL;
  }
  long h = syscall(DAVEOS_SYS_DLOPEN, path, mode);
  if (h < 0) {
    errno = ENOENT;
    return NULL;
  }
  return (void*)(uintptr_t)h;
}

void* dlsym(void* handle, const char* symbol) {
  if (symbol == NULL) {
    errno = EINVAL;
    return NULL;
  }
  long addr = syscall(DAVEOS_SYS_DLSYM, handle, symbol);
  if (addr <= 0) {
    errno = EINVAL;
    return NULL;
  }
  return (void*)(uintptr_t)addr;
}

int dlclose(void* handle) {
  if (handle == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_DLCLOSE, handle));
}

char* dlerror(void) {
  return dl_error_buf[0] != '\0' ? dl_error_buf : (char*)"dlerror";
}

static long sysconf_rlimit_soft(int resource, long fallback) {
  struct rlimit lim;
  if (getrlimit(resource, &lim) != 0) {
    return fallback;
  }
  if (lim.rlim_cur == RLIM_INFINITY) {
    return fallback;
  }
  return (long)lim.rlim_cur;
}

static long sysconf_meminfo(int which) {
  struct daveos_meminfo info;
  if (syscall(DAVEOS_SYS_GETMEMINFO, &info) < 0) {
    errno = EIO;
    return -1;
  }
  if (which == 0) {
    return (long)info.total_pages;
  }
  return (long)info.free_pages;
}

/*
 * DaveOS sysconf. Option names return 1 when supported, -1 with errno
 * unchanged when not. Invalid names return -1 with errno=EINVAL.
 * PAGE_SIZE / CLK_TCK / OPEN_MAX / CHILD_MAX / PHYS_PAGES match the kernel.
 */
long sysconf(int name) {
  switch (name) {
    case _SC_ARG_MAX:
      return 131072; /* 128 KiB; matches exec string budget in practice */
    case _SC_CHILD_MAX:
      return sysconf_rlimit_soft(RLIMIT_NPROC, DAVEOS_PID_MAX_DEFAULT);
    case _SC_CLK_TCK:
      return 100; /* 10 ms scheduler tick */
    case _SC_NGROUPS_MAX:
      return 0;
    case _SC_OPEN_MAX:
      return sysconf_rlimit_soft(RLIMIT_NOFILE, 64);
    case _SC_STREAM_MAX:
      return sysconf_rlimit_soft(RLIMIT_NOFILE, 64);
    case _SC_TZNAME_MAX:
      return 6;
    case _SC_JOB_CONTROL:
    case _SC_SAVED_IDS:
    case _SC_REALTIME_SIGNALS:
    case _SC_ASYNCHRONOUS_IO:
    case _SC_PRIORITIZED_IO:
    case _SC_SYNCHRONIZED_IO:
    case _SC_FSYNC:
    case _SC_MEMLOCK:
    case _SC_MEMLOCK_RANGE:
    case _SC_MESSAGE_PASSING:
    case _SC_SEMAPHORES:
    case _SC_SPAWN:
      return -1; /* not supported; errno unchanged */
    case _SC_THREADS:
      return 1;
    case _SC_PRIORITY_SCHEDULING:
    case _SC_TIMERS:
    case _SC_MAPPED_FILES:
    case _SC_MEMORY_PROTECTION:
    case _SC_SHARED_MEMORY_OBJECTS:
    case _SC_POLL:
    case _SC_SELECT:
    case _SC_MONOTONIC_CLOCK:
    case _SC_SHELL:
    case _SC_REGEXP:
    case _SC_XOPEN_SHM:
      return 1;
    case _SC_VERSION:
    case _SC_2_VERSION:
      return 200809L;
    case _SC_PAGESIZE:
      return 4096;
    case _SC_RTSIG_MAX:
      return 0;
    case _SC_SIGQUEUE_MAX:
      return 0;
    case _SC_TIMER_MAX:
      return 64;
    case _SC_BC_BASE_MAX:
      return 99;
    case _SC_BC_DIM_MAX:
      return 2048;
    case _SC_BC_SCALE_MAX:
      return 99;
    case _SC_BC_STRING_MAX:
      return 1000;
    case _SC_COLL_WEIGHTS_MAX:
      return 2;
    case _SC_EXPR_NEST_MAX:
      return 32;
    case _SC_LINE_MAX:
      return 2048;
    case _SC_RE_DUP_MAX:
      return 255;
    case _SC_IOV_MAX:
      return 1024;
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
      return 1;
    case _SC_PHYS_PAGES:
      return sysconf_meminfo(0);
    case _SC_AVPHYS_PAGES:
      return sysconf_meminfo(1);
    case _SC_ATEXIT_MAX:
      return 32;
    case _SC_GETPW_R_SIZE_MAX:
    case _SC_GETGR_R_SIZE_MAX:
      return -1; /* indeterminate */
    case _SC_LOGIN_NAME_MAX:
      return 256;
    case _SC_TTY_NAME_MAX:
      return 32;
    case _SC_HOST_NAME_MAX:
      return 64;
    case _SC_THREAD_THREADS_MAX:
      return 4096; /* DAVEOS_THREADS_CAP */
    case _SC_XOPEN_VERSION:
      return 700;
    case _SC_XOPEN_UNIX:
      return 1;
    default:
      errno = EINVAL;
      return -1;
  }
}

#endif /* __DAVECC_DAVEOS__ */
