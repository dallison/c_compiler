// Host-libSystem shims for the Darwin Mach-O libc.  Static destructors
// go through __cxa_atexit; .eh_frame is omitted from MH_OBJECT files.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

enum { kDarwinClockRealtime = 0, kDarwinClockMonotonic = 6 };

int clock_gettime(int clock_id, struct timespec* value);

int __cxa_atexit(void (*destructor)(void*), void* object, void* dso);

typedef struct {
  void* destructor;
  unsigned char* object;
  unsigned long count;
  unsigned long stride;
  unsigned char complete_object_argument;
} CXXDestructorContext;

static void RunCXXDestructors(void* argument) {
  CXXDestructorContext* context = argument;
  unsigned char* object = context->object;
  for (unsigned long i = 0; i < context->count; i++) {
    object += context->stride;
  }
  while (context->count != 0) {
    context->count--;
    object -= context->stride;
    if (context->complete_object_argument) {
      ((void (*)(void*, int))context->destructor)(object, 1);
    } else {
      ((void (*)(void*))context->destructor)(object);
    }
  }
  free(context);
}

int __davecc_cxa_atexit(void* destructor, void* object, unsigned long count,
                        unsigned long stride, int complete_object_argument) {
  CXXDestructorContext* context = malloc(sizeof(CXXDestructorContext));
  if (context == NULL) {
    return -1;
  }
  context->destructor = destructor;
  context->object = object;
  context->count = count;
  context->stride = stride;
  context->complete_object_argument = complete_object_argument != 0;
  if (__cxa_atexit(RunCXXDestructors, context, NULL) != 0) {
    free(context);
    return -1;
  }
  return 0;
}

void __davecc_atexit_lock(unsigned int* lock) {
  (void)lock;
}

void __davecc_atexit_unlock(unsigned int* lock) {
  (void)lock;
}

char __eh_frame_start[1];
char __eh_frame_end[1];

long long __davecc_monotonic_time_us(void) {
  struct timespec value;
  if (clock_gettime(kDarwinClockMonotonic, &value) == 0) {
    return (long long)value.tv_sec * 1000000 + value.tv_nsec / 1000;
  }
  return 0;
}

long long __davecc_realtime_time_us(void) {
  struct timespec value;
  if (clock_gettime(kDarwinClockRealtime, &value) == 0) {
    return (long long)value.tv_sec * 1000000 + value.tv_nsec / 1000;
  }
  return (long long)time(NULL) * 1000000;
}
