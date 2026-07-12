//
//  threads.h
//  libc
//
//  Minimal C11 <threads.h> for DaveCC guest programs.
//

#ifndef threads_h
#define threads_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long thrd_t;
typedef int (*thrd_start_t)(void*);

enum {
  thrd_success = 0,
  thrd_nomem = 1,
  thrd_timedout = 2,
  thrd_busy = 3,
  thrd_error = 4
};

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg);
int thrd_join(thrd_t thr, int* res);
thrd_t thrd_current(void);
int thrd_equal(thrd_t a, thrd_t b);
void thrd_exit(int res);

#ifdef __cplusplus
}
#endif

#endif /* threads_h */
