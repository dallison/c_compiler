#include <threads.h>
#include <time.h>

static mtx_t mutex;
static cnd_t condition = CND_INITIALIZER;
static int waiting;
static int ready;
static int awakened;

static int wait_worker(void* argument) {
  (void)argument;
  if (mtx_lock(&mutex) != thrd_success) {
    return 1;
  }
  waiting++;
  while (!ready) {
    if (cnd_wait(&condition, &mutex) != thrd_success) {
      mtx_unlock(&mutex);
      return 2;
    }
  }
  awakened++;
  mtx_unlock(&mutex);
  return 0;
}

static int read_counter(int* counter) {
  int value;
  mtx_lock(&mutex);
  value = *counter;
  mtx_unlock(&mutex);
  return value;
}

int main(void) {
  if (mtx_init(&mutex, mtx_plain) != thrd_success ||
      cnd_init(&condition) != thrd_success) {
    return 1;
  }

  thrd_t first;
  thrd_t second;
  if (thrd_create(&first, wait_worker, 0) != thrd_success ||
      thrd_create(&second, wait_worker, 0) != thrd_success) {
    return 2;
  }
  while (read_counter(&waiting) != 2) {
    thrd_yield();
  }

  mtx_lock(&mutex);
  ready = 1;
  if (cnd_signal(&condition) != thrd_success) {
    mtx_unlock(&mutex);
    return 3;
  }
  mtx_unlock(&mutex);
  while (read_counter(&awakened) == 0) {
    thrd_yield();
  }

  if (cnd_broadcast(&condition) != thrd_success) {
    return 4;
  }
  int first_result = 0;
  int second_result = 0;
  if (thrd_join(first, &first_result) != thrd_success ||
      thrd_join(second, &second_result) != thrd_success ||
      first_result != 0 || second_result != 0 || awakened != 2) {
    return 5;
  }

  mtx_lock(&mutex);
  struct timespec expired = {time(0), 0};
  int timeout_result = cnd_timedwait(&condition, &mutex, &expired);
  mtx_unlock(&mutex);
  if (timeout_result != thrd_timedout) {
    return 6;
  }

  long long deadline_us = __davecc_realtime_time_us() + 2000;
  if (deadline_us < 0) {
    return 23;
  }
  long long seconds = deadline_us / 1000000;
  long long microseconds = deadline_us % 1000000;
  if (seconds < 0 || microseconds < 0 || microseconds >= 1000000) {
    return 21;
  }
  long nanoseconds = (long)(microseconds * 1000);
  if (nanoseconds < 0 || nanoseconds >= 1000000000) {
    return 22;
  }
  struct timespec future = {
      (time_t)seconds,
      nanoseconds,
  };
  if (future.tv_sec < 0 || future.tv_nsec < 0 ||
      future.tv_nsec >= 1000000000) {
    return 20;
  }
  long long before = __davecc_monotonic_time_us();
  mtx_lock(&mutex);
  timeout_result = cnd_timedwait(&condition, &mutex, &future);
  int unlock_result = mtx_unlock(&mutex);
  long long elapsed = __davecc_monotonic_time_us() - before;
  cnd_destroy(&condition);
  mtx_destroy(&mutex);
  if (unlock_result != thrd_success) {
    return 60 + unlock_result;
  }
  if (timeout_result != thrd_timedout) {
    return 7;
  }
  return elapsed >= 1000 ? 0 : 8;
}
