//
//  pthread.h
//  libc
//
//  Minimal POSIX threads declarations so C++ headers that include <pthread.h>
//  (Abseil's thread identity and waiter) can be parsed.  These are declarations
//  only; guest programs that call them still need a target implementation.
//

#ifndef pthread_h
#define pthread_h

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long pthread_t;
typedef unsigned int pthread_key_t;

typedef struct {
  unsigned int state;
  pthread_t owner;
  unsigned int recursion;
  int type;
} pthread_mutex_t;

typedef struct {
  unsigned int generation;
} pthread_cond_t;

typedef struct {
  int dummy;
} pthread_mutexattr_t;

typedef struct {
  int dummy;
} pthread_condattr_t;

typedef struct {
  int dummy;
} pthread_attr_t;

#define PTHREAD_MUTEX_INITIALIZER {0, 0, 0, 0}
#define PTHREAD_COND_INITIALIZER {0}
#define PTHREAD_ONCE_INIT 0

typedef int pthread_once_t;

int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);
int pthread_mutex_trylock(pthread_mutex_t*);

int pthread_cond_init(pthread_cond_t*, const pthread_condattr_t*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_broadcast(pthread_cond_t*);
int pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*,
                           const struct timespec*);

int pthread_key_create(pthread_key_t*, void (*)(void*));
int pthread_key_delete(pthread_key_t);
void* pthread_getspecific(pthread_key_t);
int pthread_setspecific(pthread_key_t, const void*);

int pthread_once(pthread_once_t*, void (*)(void));
// Apple's pthread_t is a pointer; NULL names the calling thread.
int pthread_threadid_np(const void* thread, unsigned long long* thread_id);
pthread_t pthread_self(void);
int pthread_equal(pthread_t, pthread_t);
int pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
int pthread_join(pthread_t, void**);
int pthread_detach(pthread_t);

#ifdef __cplusplus
}
#endif

#endif /* pthread_h */
