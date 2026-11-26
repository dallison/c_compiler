#ifndef sys_wait_h
#define sys_wait_h

#if defined(__DAVECC_DAVEOS__)
#include <signal.h>
#endif

#ifndef __pid_t
#define __pid_t
typedef int pid_t;
#endif

#if defined(__DAVECC_DAVEOS__)
#ifndef __id_t
#define __id_t
typedef unsigned id_t;
#endif
#endif

#define WNOHANG 1
#define WUNTRACED 2
#define WCONTINUED 8
#if defined(__DAVECC_DAVEOS__)
#define WEXITED 4
#endif

#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WTERMSIG(status) ((status) & 0x7f)
#define WSTOPSIG(status) WEXITSTATUS(status)
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#define WIFSIGNALED(status) \
  (((signed char)(((status) & 0x7f) + 1) >> 1) > 0)
#define WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#define WIFCONTINUED(status) ((status) == 0xffff)

#if defined(__DAVECC_DAVEOS__)
typedef enum {
  P_ALL = 0,
  P_PID = 1,
  P_PGID = 2,
  P_PIDFD = 3
} idtype_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

pid_t waitpid(pid_t pid, int* status, int options);
#if defined(__DAVECC_DAVEOS__)
int waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options);
#endif

#ifdef __cplusplus
}
#endif

#endif
