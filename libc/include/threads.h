//
//  threads.h
//  libc
//
//  Minimal C11 <threads.h> for DaveCC guest programs.
//

#ifndef threads_h
#define threads_h

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long thrd_t;
typedef int (*thrd_start_t)(void*);
typedef struct {
  unsigned int state;
  thrd_t owner;
  unsigned int recursion;
  int type;
} mtx_t;

typedef struct {
  unsigned int state;
} dave_once_flag_t;

#ifndef __cplusplus
typedef dave_once_flag_t once_flag;
#define ONCE_FLAG_INIT {0}
#endif

enum {
  mtx_plain = 0,
  mtx_recursive = 1,
  mtx_timed = 2
};

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
void thrd_yield(void);

int mtx_init(mtx_t* mutex, int type);
void mtx_destroy(mtx_t* mutex);
int mtx_lock(mtx_t* mutex);
int mtx_trylock(mtx_t* mutex);
int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point);
int mtx_unlock(mtx_t* mutex);
#ifndef __cplusplus
void call_once(once_flag* flag, void (*func)(void));
#endif

long long __davecc_monotonic_time_us(void);
int __davecc_mtx_timedlock_for(mtx_t* mutex, long long timeout_us);
int __davecc_once_begin(dave_once_flag_t* flag);
void __davecc_once_complete(dave_once_flag_t* flag);
void __davecc_once_abort(dave_once_flag_t* flag);

#ifdef __cplusplus
}
#endif

#endif /* threads_h */
