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
#define RLIMIT_MEMLOCK 6
#define RLIMIT_NPROC 7

#ifdef __cplusplus
extern "C" {
#endif

int getrlimit(int resource, struct rlimit* limit);
int setrlimit(int resource, const struct rlimit* limit);

#ifdef __cplusplus
}
#endif

#endif
