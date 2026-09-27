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

#define SIGABRT 1
#define SIGFPE 2
#define SIGILL 3
#define SIGINT 4
#define SIGSEGV 5
#define SIGTERM 6

typedef unsigned sigset_t;

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

#ifdef __cplusplus
}
#endif

#endif /* __davecc_signal_h__ */
