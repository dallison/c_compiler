//
//  signal.c
//  c_compiler
//

#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <syscall.h>

#if defined(__DAVECC_NATIVE_LINUX__)
#define DAVECC_MAX_SIGNAL 64
#else
#define DAVECC_MAX_SIGNAL SIGTERM
#endif

static void (*handlers[DAVECC_MAX_SIGNAL + 1])(int);

static int valid_signal(int signal_number) {
  return signal_number > 0 && signal_number <= DAVECC_MAX_SIGNAL;
}

void (*signal(int signal_number, void (*handler)(int)))(int) {
  void (*previous)(int);
  if (!valid_signal(signal_number) || handler == SIG_ERR) {
    return SIG_ERR;
  }
  previous = handlers[signal_number];
  handlers[signal_number] = handler;
  return previous;
}

int raise(int signal_number) {
  void (*handler)(int);
  if (!valid_signal(signal_number)) {
    return 1;
  }
  handler = handlers[signal_number];
  if (handler == SIG_IGN) {
    return 0;
  }
  if (handler == SIG_DFL) {
    abort();
  }
  handler(signal_number);
  return 0;
}

int sigemptyset(sigset_t* set) {
  if (set == NULL) {
    errno = EINVAL;
    return -1;
  }
  *set = 0;
  return 0;
}

int sigfillset(sigset_t* set) {
  if (set == NULL) {
    errno = EINVAL;
    return -1;
  }
  *set = ~(sigset_t)0;
  return 0;
}

int sigaddset(sigset_t* set, int signal_number) {
  if (set == NULL || signal_number <= 0 ||
      signal_number > (int)(8 * sizeof(*set))) {
    errno = EINVAL;
    return -1;
  }
  *set |= (sigset_t)1 << (signal_number - 1);
  return 0;
}

int sigdelset(sigset_t* set, int signal_number) {
  if (set == NULL || signal_number <= 0 ||
      signal_number > (int)(8 * sizeof(*set))) {
    errno = EINVAL;
    return -1;
  }
  *set &= ~((sigset_t)1 << (signal_number - 1));
  return 0;
}

int sigismember(const sigset_t* set, int signal_number) {
  if (set == NULL || signal_number <= 0 ||
      signal_number > (int)(8 * sizeof(*set))) {
    errno = EINVAL;
    return -1;
  }
  return (*set & ((sigset_t)1 << (signal_number - 1))) != 0;
}

int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) {
#if defined(__DAVECC_NATIVE_LINUX__)
  long result =
      syscall(SYS_rt_sigprocmask, how, set, oldset, sizeof(sigset_t));
  return result < 0 ? (int)-result : 0;
#else
  (void)how;
  (void)set;
  if (oldset != NULL) {
    *oldset = 0;
  }
  return 0;
#endif
}
