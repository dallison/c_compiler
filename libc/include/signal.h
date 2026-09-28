//
//  signal.h
//  c_compiler
//

#ifndef __davecc_signal_h__
#define __davecc_signal_h__

typedef int sig_atomic_t;

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#if defined(__DAVECC_NATIVE_DARWIN__)
#define SIGINT 2
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGFPE 8
#define SIGBUS 10
#define SIGSEGV 11
#define SIGALRM 14
#define SIGTERM 15
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
#endif

typedef unsigned sigset_t;

#define SA_ONSTACK 0x0001
#define SA_RESETHAND 0x0004
#define SA_NODEFER 0x0010
#define SA_SIGINFO 0x0040

typedef struct {
  int si_signo;
  int si_errno;
  int si_code;
} siginfo_t;

struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
  };
  sigset_t sa_mask;
  int sa_flags;
};

#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

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
int sigaction(int signal_number, const struct sigaction* action,
              struct sigaction* old_action);
unsigned alarm(unsigned seconds);

#ifdef __cplusplus
}
#endif

#endif /* __davecc_signal_h__ */
