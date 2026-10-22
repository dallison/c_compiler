#ifndef davecc_sys_timerfd_h
#define davecc_sys_timerfd_h

#include <time.h>

struct itimerspec {
  struct timespec it_interval;
  struct timespec it_value;
};

#define TFD_TIMER_ABSTIME 1
#define TFD_CLOEXEC 02000000
#define TFD_NONBLOCK 00004000

#ifdef __cplusplus
extern "C" {
#endif

int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags, const struct itimerspec* new_value,
                    struct itimerspec* old_value);
int timerfd_gettime(int fd, struct itimerspec* curr_value);

#ifdef __cplusplus
}
#endif

#endif
