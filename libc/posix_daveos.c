/*
 * DaveOS libc wrappers: real svc calls for fork/eventfd/epoll/… .
 * Built only when compiling with -target aarch64-unknown-daveos-davecc.
 */

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
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

int daveos_exec(const char* name) {
  if (name == NULL) {
    errno = EINVAL;
    return -1;
  }
  return Fail(syscall(DAVEOS_SYS_EXEC, name));
}

int execv(const char* path, char** argv) {
  (void)argv;
  return daveos_exec(path);
}

int execve(const char* path, char** argv, char** envp) {
  (void)argv;
  (void)envp;
  return daveos_exec(path);
}

int sendfd(int pipe_fd, int fd) {
  return Fail(syscall(DAVEOS_SYS_SENDFD, pipe_fd, fd));
}

int recvfd(int pipe_fd) {
  return Fail(syscall(DAVEOS_SYS_RECVFD, pipe_fd));
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

#endif /* __DAVECC_DAVEOS__ */
