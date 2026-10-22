#ifndef sys_resource_h
#define sys_resource_h

// Enough of POSIX <sys/resource.h> for getrlimit(RLIMIT_NOFILE).

typedef unsigned long rlim_t;

struct rlimit {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};

#define RLIMIT_CPU 0
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_AS 5
#define RLIMIT_RSS 5
#define RLIMIT_NOFILE 8
#define RLIM_INFINITY ((rlim_t)-1)
#define RLIMIT_MEMLOCK 6
#define RLIMIT_NPROC 7

#if defined(__DAVECC_NATIVE_DARWIN__)
/* Flavors for libSystem's proc_pid_rusage(), which <libproc.h> declares. */
#define RUSAGE_INFO_V0 0
#define RUSAGE_INFO_V1 1
#define RUSAGE_INFO_V2 2
#define RUSAGE_INFO_V3 3
#define RUSAGE_INFO_V4 4
#define RUSAGE_INFO_V5 5
#define RUSAGE_INFO_V6 6
#define RUSAGE_INFO_CURRENT RUSAGE_INFO_V6

typedef void* rusage_info_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int getrlimit(int resource, struct rlimit* limit);
int setrlimit(int resource, const struct rlimit* limit);

#ifdef __cplusplus
}
#endif

#endif
