//
//  signal.h
//  c_compiler
//

#ifndef __davecc_signal_h__
#define __davecc_signal_h__

#if defined(__DAVECC_NATIVE_DARWIN__)
// libSystem implements the signal API; its siginfo_t, stack_t, and sigaction
// layouts are Darwin's own.
#include_next <signal.h>
#else

typedef int sig_atomic_t;

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#if defined(__DAVECC_NATIVE_LINUX__) || defined(__DAVECC_DAVEOS__)
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#else
#define SIGABRT 1
#define SIGFPE 2
#define SIGILL 3
#define SIGINT 4
#define SIGSEGV 5
#define SIGTERM 6
#define SIGTRAP 7
#define SIGBUS 8
#define SIGALRM 9
#define SIGHUP 10
#define SIGQUIT 11
#define SIGKILL 12
#define SIGPIPE 13
#define SIGUSR1 14
#define SIGUSR2 15
#define SIGCHLD 16
#define SIGCONT 17
#define SIGSTOP 18
#define SIGTSTP 19
#define SIGPROF 27
#endif

#define SIGSTKSZ 8192
#define MINSIGSTKSZ 2048

#if defined(__DAVECC_NATIVE_LINUX__) || defined(__DAVECC_DAVEOS__)
typedef unsigned long sigset_t;
#else
typedef unsigned sigset_t;
#endif

typedef struct {
  void* ss_sp;
  int ss_flags;
  unsigned long ss_size;
} stack_t;

#if defined(__DAVECC_DAVEOS__)
/* Match DaveOS kernel sa_flags (not Linux values). */
#define SA_RESTART 0x00000001
#define SA_NODEFER 0x00000002
#define SA_SIGINFO 0x00000004
#define SA_ONSTACK 0x00000008
#define SA_RESETHAND 0x00000010
#elif defined(__DAVECC_NATIVE_LINUX__)
#define SA_SIGINFO 0x00000004
#define SA_ONSTACK 0x08000000
#define SA_RESTART 0x10000000
#define SA_NODEFER 0x40000000
#define SA_RESETHAND 0x80000000
#else
#define SA_ONSTACK 0x0001
#define SA_RESETHAND 0x0004
#define SA_NODEFER 0x0010
#define SA_SIGINFO 0x0040
#endif

typedef struct {
  int si_signo;
  int si_errno;
  int si_code;
#if defined(__DAVECC_DAVEOS__)
  /* waitid(2) fields (DaveOS subset; matches kernel DaveosSiginfo). */
  int si_pid;
  int si_uid;
  int si_status;
#endif
} siginfo_t;

#if defined(__DAVECC_DAVEOS__)
#define CLD_EXITED 1
#define CLD_KILLED 2
#endif

struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
  };
  sigset_t sa_mask;
  int sa_flags;
};

#if defined(__DAVECC_NATIVE_LINUX__) || defined(__DAVECC_DAVEOS__)
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
#else
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3
#endif

#ifdef __cplusplus
extern "C" {
#endif

void (*signal(int signal_number, void (*handler)(int)))(int);
int raise(int signal_number);
int sigemptyset(sigset_t* set);
int sigfillset(sigset_t* set);
int sigaddset(sigset_t* set, int signal_number);
int sigdelset(sigset_t* set, int signal_number);
int sigismember(const sigset_t* set, int signal_number);
int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset);
int sigprocmask(int how, const sigset_t* set, sigset_t* oldset);
int sigaction(int signal_number, const struct sigaction* action,
              struct sigaction* old_action);
int sigaltstack(const stack_t* stack, stack_t* old_stack);
unsigned alarm(unsigned seconds);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_NATIVE_DARWIN__ */

#endif /* __davecc_signal_h__ */
