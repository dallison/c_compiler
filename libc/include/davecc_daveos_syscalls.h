#ifndef davecc_daveos_syscalls_h
#define davecc_daveos_syscalls_h

/*
 * DaveOS-local syscall numbers (svc #0, number in x16).
 * Shared with the DaveOS kernel (kernel/daveos_syscalls.h). Guest ABI
 * numbers (DAVE_SYS_*) remain for open/read/write/pipe/mmap/getpid/…
 */

#define DAVEOS_SYS_FORK 200
#define DAVEOS_SYS_WAITPID 201
#define DAVEOS_SYS_BRK 202
#define DAVEOS_SYS_EXEC 203
#define DAVEOS_SYS_KILL 204
#define DAVEOS_SYS_SIGACTION 205
#define DAVEOS_SYS_SIGRETURN 206
#define DAVEOS_SYS_SIGPROCMASK 207
#define DAVEOS_SYS_EVENTFD 208
#define DAVEOS_SYS_TIMERFD_CREATE 209
#define DAVEOS_SYS_TIMERFD_SETTIME 210
#define DAVEOS_SYS_TIMERFD_GETTIME 211
#define DAVEOS_SYS_POLL 212
#define DAVEOS_SYS_SELECT 213
#define DAVEOS_SYS_EPOLL_CREATE1 214
#define DAVEOS_SYS_EPOLL_CTL 215
#define DAVEOS_SYS_EPOLL_WAIT 216
#define DAVEOS_SYS_SENDFD 217
#define DAVEOS_SYS_RECVFD 218
#define DAVEOS_SYS_GETPROCS 219
#define DAVEOS_SYS_MOUNT 220
#define DAVEOS_SYS_UMOUNT 221

/* mount(2) flags (Linux values). */
#define DAVEOS_MS_BIND 4096
#ifndef MS_BIND
#define MS_BIND DAVEOS_MS_BIND
#endif

/* Kernel select bitmap — fds 0..63. */
#define DAVEOS_FD_SETSIZE 64

struct daveos_proc_info {
  unsigned pid;
  unsigned ppid;
  unsigned state; /* 0=live, 1=zombie, 2=stopped */
  char name[16];
};

struct daveos_fd_set {
  unsigned long long mask;
};

/* Kernel sigaction layout (not identical to libc struct sigaction). */
struct daveos_sigaction {
  unsigned long long sa_handler;
  unsigned sa_mask;
  unsigned sa_flags;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Exec ELF at VFS |path|; argv becomes {path, NULL}, env is environ. */
int daveos_exec(const char* path);

/* Pass / receive an open file over a pipe (SCM_RIGHTS-style). */
int sendfd(int pipe_fd, int fd);
int recvfd(int pipe_fd);

/* Copy up to |max| process rows; returns count or -1. */
int daveos_getprocs(struct daveos_proc_info* buf, int max);

/* Linux-like mount(2) / umount2(2); flags and data ignored by DaveOS. */
int mount(const char* source, const char* target, const char* filesystemtype,
          unsigned long mountflags, const void* data);
int umount2(const char* target, int flags);

void* sbrk(long increment);

#ifdef __cplusplus
}
#endif

#endif /* davecc_daveos_syscalls_h */
