#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Keep the public type large enough for Linux's process-local semaphore
// representation.  The synchronization implementation selected by Abseil on
// Linux uses futexes, but its alternate waiter type still requires sem_t to be
// complete while headers are parsed.
typedef union {
  char __storage[32];
  long __alignment;
} sem_t;

#define SEM_FAILED ((sem_t*)0)

int sem_init(sem_t* sem, int pshared, unsigned int value);
int sem_destroy(sem_t* sem);
int sem_wait(sem_t* sem);
int sem_trywait(sem_t* sem);
int sem_timedwait(sem_t* sem, const struct timespec* absolute_timeout);
int sem_post(sem_t* sem);
int sem_getvalue(sem_t* sem, int* value);

#ifdef __cplusplus
}
#endif

#endif
