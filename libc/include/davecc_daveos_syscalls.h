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
#define DAVEOS_SYS_GETMOUNTS 222
#define DAVEOS_SYS_PRLIMIT64 223
#define DAVEOS_SYS_GETMEMINFO 224
#define DAVEOS_SYS_SYSLIMIT 225
#define DAVEOS_SYS_DLOPEN 226
#define DAVEOS_SYS_DLSYM 227
#define DAVEOS_SYS_DLCLOSE 228
#define DAVEOS_SYS_CLOCK_GETTIME 229
#define DAVEOS_SYS_GETTIMEOFDAY 230
#define DAVEOS_SYS_SETSID 231
#define DAVEOS_SYS_IOCTL 232
#define DAVEOS_SYS_GETUID 233
#define DAVEOS_SYS_GETEUID 234
#define DAVEOS_SYS_GETGID 235
#define DAVEOS_SYS_GETEGID 236
#define DAVEOS_SYS_GETGROUPS 237
#define DAVEOS_SYS_SETUID 238
#define DAVEOS_SYS_SETEUID 239
#define DAVEOS_SYS_SETGID 240
#define DAVEOS_SYS_SETEGID 241
#define DAVEOS_SYS_SETGROUPS 242
#define DAVEOS_SYS_INIT_MODULE 243
#define DAVEOS_SYS_DELETE_MODULE 244
#define DAVEOS_SYS_NETIF 245
#define DAVEOS_SYS_PING 246
#define DAVEOS_SYS_CLONE 247
#define DAVEOS_SYS_PIDFD_OPEN 248
#define DAVEOS_SYS_PIDFD_SEND_SIGNAL 249
#define DAVEOS_SYS_WAITID 250
#define DAVEOS_SYS_NETSTAT 251
#define DAVEOS_SYS_CLOCK_GETRES 252
#define DAVEOS_SYS_GETCPU 253
#define DAVEOS_SYS_STATFS 254
#define DAVEOS_SYS_SUBSPACE_OPEN 255
#define DAVEOS_SYS_SUBSPACE_CALL 256
#define DAVEOS_SYS_SUBSPACE_CLOSE 257

#define DAVEOS_NETIF_GET 0
#define DAVEOS_NETIF_SET 1

/* Linux-compatible clone / pidfd bits (DaveOS subset). */
#define CLONE_PIDFD 0x1000
#define PIDFD_NONBLOCK 0x800

#define DAVEOS_NETSTAT_TCP 6
#define DAVEOS_NETSTAT_UDP 17

struct daveos_netif {
  char name[16];
  unsigned addr;
  unsigned netmask;
  unsigned gateway;
  unsigned flags; /* bit0 = configured/up */
};

/* Host-endian; TCP state matches kernel TcpSock::State; UDP 0=bound 1=connected. */
struct daveos_sockinfo {
  unsigned proto;
  unsigned state;
  unsigned local_addr;
  unsigned remote_addr;
  unsigned short local_port;
  unsigned short remote_port;
};

#ifdef __cplusplus
extern "C" {
#endif
int init_module(const char* path);
int delete_module(const char* name);
int daveos_netif(int op, struct daveos_netif* nif);
int daveos_ping(unsigned addr, unsigned timeout_ms);
/* Copy up to |max| AF_INET socket rows; returns count or -1. */
int daveos_netstat(struct daveos_sockinfo* buf, int max);
/*
 * Linux clone(2) syscall ABI (aarch64 argument order). With CLONE_PIDFD,
 * *parent_tid receives the new pidfd. Returns child pid to the parent, 0
 * in the child.
 */
long daveos_clone(unsigned long flags, void* child_stack, int* parent_tid,
                  void* tls, int* child_tid);
int pidfd_open(int pid, unsigned int flags);
int pidfd_send_signal(int pidfd, int sig, void* info, unsigned int flags);
#ifdef __cplusplus
}
#endif

/* TTY ioctls (Linux aarch64 values). */
#define DAVEOS_TIOCSCTTY 0x540E
#define DAVEOS_TIOCGWINSZ 0x5413

/* mount(2) flags (Linux values). */
#define DAVEOS_MS_BIND 4096

#define DAVEOS_LIMIT_PID_MAX 1
#define DAVEOS_LIMIT_THREADS_MAX 2
#define DAVEOS_PID_CAP 4096
#define DAVEOS_THREADS_CAP 4096
#define DAVEOS_PID_MAX_DEFAULT 256
#define DAVEOS_THREADS_MAX_DEFAULT 256

/* RLIMIT_* ids — keep in sync with <sys/resource.h>. */
#define DAVEOS_RLIMIT_CPU 0
#define DAVEOS_RLIMIT_FSIZE 1
#define DAVEOS_RLIMIT_DATA 2
#define DAVEOS_RLIMIT_STACK 3
#define DAVEOS_RLIMIT_CORE 4
#define DAVEOS_RLIMIT_AS 5
#define DAVEOS_RLIMIT_MEMLOCK 6
#define DAVEOS_RLIMIT_NPROC 7
#define DAVEOS_RLIMIT_NOFILE 8
#define DAVEOS_RLIMIT_NLIMITS 9
#define DAVEOS_RLIM_INFINITY (~(unsigned long long)0)

struct daveos_rlimit {
  unsigned long long soft;
  unsigned long long hard;
};

struct daveos_meminfo {
  unsigned long long page_size;
  unsigned long long total_pages;
  unsigned long long free_pages;
};
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

struct daveos_mount_info {
  char source[64];
  char target[64];
  char fstype[16];
};

struct daveos_statfs {
  unsigned long long bsize;
  unsigned long long blocks;
  unsigned long long bfree;
  unsigned long long bavail;
  unsigned long long files;
  unsigned long long ffree;
  char fstype[16];
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

/* Copy up to |max| mount-table rows; returns count or -1. */
int daveos_getmounts(struct daveos_mount_info* buf, int max);

/* Filesystem capacity for |path|; returns 0 or -1. */
int daveos_statfs(const char* path, struct daveos_statfs* buf);

/* Linux-like mount(2) / umount2(2); flags and data ignored by DaveOS. */
int mount(const char* source, const char* target, const char* filesystemtype,
          unsigned long mountflags, const void* data);
int umount2(const char* target, int flags);

void* sbrk(long increment);

/*
 * Query/set system-wide pid_max / threads_max.
 * which: DAVEOS_LIMIT_PID_MAX or DAVEOS_LIMIT_THREADS_MAX.
 * new_limit may be NULL (query only); old_limit may be NULL.
 */
int daveos_syslimit(int which, unsigned long long* new_limit,
                    unsigned long long* old_limit);

#ifdef __cplusplus
}
#endif

#endif /* davecc_daveos_syscalls_h */
