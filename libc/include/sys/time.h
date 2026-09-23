#ifndef sys_time_h
#define sys_time_h

#include <time.h>

struct timeval {
  time_t tv_sec;
  long tv_usec;
};

#endif
