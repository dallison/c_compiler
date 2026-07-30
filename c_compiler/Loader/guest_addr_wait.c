#include "guest_addr_wait.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct GuestAddrWaitBucket {
  uint64_t guest_address;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  size_t waiter_count;
  GuestAddrWaitBucket* next;
};

static size_t WaitBucketIndex(uint64_t guest_address) {
  uint64_t mixed = guest_address ^ (guest_address >> 17);
  return (size_t)(mixed & (GUEST_ADDR_WAIT_BUCKETS - 1));
}

static GuestAddrWaitBucket* FindBucketLocked(GuestAddrWaitTable* table,
                                             uint64_t guest_address) {
  size_t index = WaitBucketIndex(guest_address);
  for (GuestAddrWaitBucket* bucket = table->buckets[index];
       bucket != NULL; bucket = bucket->next) {
    if (bucket->guest_address == guest_address) {
      return bucket;
    }
  }
  return NULL;
}

static GuestAddrWaitBucket* GetBucket(GuestAddrWaitTable* table,
                                      uint64_t guest_address, bool create) {
  pthread_mutex_lock(&table->mutex);
  GuestAddrWaitBucket* bucket = FindBucketLocked(table, guest_address);
  if (bucket == NULL && create &&
      !__atomic_load_n(&table->shutting_down, __ATOMIC_ACQUIRE)) {
    bucket = calloc(1, sizeof(*bucket));
    if (bucket != NULL) {
      bucket->guest_address = guest_address;
      if (pthread_mutex_init(&bucket->mutex, NULL) != 0) {
        free(bucket);
        bucket = NULL;
      } else {
        pthread_condattr_t attributes;
        int attr_result = pthread_condattr_init(&attributes);
        bool attr_initialized = attr_result == 0;
#if !defined(__APPLE__)
        if (attr_result == 0) {
          attr_result = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
        }
#endif
        int cond_result =
            attr_result == 0
                ? pthread_cond_init(&bucket->condition, &attributes)
                : attr_result;
        if (attr_initialized) {
          pthread_condattr_destroy(&attributes);
        }
        if (cond_result != 0) {
          pthread_mutex_destroy(&bucket->mutex);
          free(bucket);
          bucket = NULL;
        } else {
          size_t index = WaitBucketIndex(guest_address);
          bucket->next = table->buckets[index];
          table->buckets[index] = bucket;
        }
      }
    }
  }
  pthread_mutex_unlock(&table->mutex);
  return bucket;
}

static bool ReadValueMatches(GuestAddrReadFn read, void* context,
                             uint64_t guest_address, const void* expected,
                             size_t size, bool* matches) {
  unsigned char current[8];
  if (!read(context, guest_address, current, size)) {
    return false;
  }
  *matches = memcmp(current, expected, size) == 0;
  return true;
}

static struct timespec WaitDeadline(int64_t timeout_us) {
  struct timespec deadline = {0};
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  deadline.tv_sec += timeout_us / 1000000;
  deadline.tv_nsec += (timeout_us % 1000000) * 1000;
  if (deadline.tv_nsec >= 1000000000) {
    deadline.tv_sec++;
    deadline.tv_nsec -= 1000000000;
  }
  return deadline;
}

#if defined(__APPLE__)
static bool WaitRemaining(struct timespec deadline,
                          struct timespec* remaining) {
  struct timespec now = {0};
  clock_gettime(CLOCK_MONOTONIC, &now);
  remaining->tv_sec = deadline.tv_sec - now.tv_sec;
  remaining->tv_nsec = deadline.tv_nsec - now.tv_nsec;
  if (remaining->tv_nsec < 0) {
    remaining->tv_sec--;
    remaining->tv_nsec += 1000000000;
  }
  return remaining->tv_sec >= 0;
}
#endif

bool GuestAddrWaitTableInit(GuestAddrWaitTable* table) {
  memset(table, 0, sizeof(*table));
  if (pthread_mutex_init(&table->mutex, NULL) != 0) {
    return false;
  }
  table->initialized = true;
  return true;
}

void GuestAddrWaitTableShutdown(GuestAddrWaitTable* table) {
  if (table == NULL || !table->initialized) {
    return;
  }
  pthread_mutex_lock(&table->mutex);
  __atomic_store_n(&table->shutting_down, true, __ATOMIC_RELEASE);
  for (size_t i = 0; i < GUEST_ADDR_WAIT_BUCKETS; i++) {
    for (GuestAddrWaitBucket* bucket = table->buckets[i]; bucket != NULL;
         bucket = bucket->next) {
      pthread_mutex_lock(&bucket->mutex);
      pthread_cond_broadcast(&bucket->condition);
      pthread_mutex_unlock(&bucket->mutex);
    }
  }
  pthread_mutex_unlock(&table->mutex);
}

void GuestAddrWaitTableDestruct(GuestAddrWaitTable* table) {
  if (table == NULL || !table->initialized) {
    return;
  }
  GuestAddrWaitTableShutdown(table);
  for (size_t i = 0; i < GUEST_ADDR_WAIT_BUCKETS; i++) {
    GuestAddrWaitBucket* bucket = table->buckets[i];
    while (bucket != NULL) {
      GuestAddrWaitBucket* next = bucket->next;
      pthread_cond_destroy(&bucket->condition);
      pthread_mutex_destroy(&bucket->mutex);
      free(bucket);
      bucket = next;
    }
    table->buckets[i] = NULL;
  }
  pthread_mutex_destroy(&table->mutex);
  table->initialized = false;
}

int GuestAddrWait(GuestAddrWaitTable* table, GuestAddrReadFn read,
                  void* context, uint64_t guest_address,
                  const void* expected, size_t size, int64_t timeout_us) {
  if (table == NULL || !table->initialized || read == NULL ||
      expected == NULL ||
      (size != 1 && size != 2 && size != 4 && size != 8) ||
      timeout_us < -1) {
    return kGuestAddrWaitError;
  }

  GuestAddrWaitBucket* bucket = GetBucket(table, guest_address, true);
  if (bucket == NULL) {
    return kGuestAddrWaitError;
  }

  struct timespec deadline = {0};
  if (timeout_us > 0) {
    deadline = WaitDeadline(timeout_us);
  }

  pthread_mutex_lock(&bucket->mutex);
  bool matches = false;
  if (!ReadValueMatches(read, context, guest_address, expected, size,
                        &matches)) {
    pthread_mutex_unlock(&bucket->mutex);
    return kGuestAddrWaitError;
  }
  if (!matches) {
    pthread_mutex_unlock(&bucket->mutex);
    return kGuestAddrWaitSuccess;
  }
  if (timeout_us == 0) {
    pthread_mutex_unlock(&bucket->mutex);
    return kGuestAddrWaitTimedOut;
  }

  bucket->waiter_count++;
  int result = kGuestAddrWaitSuccess;
  for (;;) {
    if (!ReadValueMatches(read, context, guest_address, expected, size,
                          &matches)) {
      result = kGuestAddrWaitError;
      break;
    }
    if (!matches) {
      break;
    }
    if (__atomic_load_n(&table->shutting_down, __ATOMIC_ACQUIRE)) {
      result = kGuestAddrWaitError;
      break;
    }
    int wait_result;
    if (timeout_us < 0) {
      wait_result = pthread_cond_wait(&bucket->condition, &bucket->mutex);
    } else {
#if defined(__APPLE__)
      struct timespec remaining;
      if (!WaitRemaining(deadline, &remaining)) {
        wait_result = ETIMEDOUT;
      } else {
        wait_result = pthread_cond_timedwait_relative_np(
            &bucket->condition, &bucket->mutex, &remaining);
      }
#else
      wait_result = pthread_cond_timedwait(&bucket->condition, &bucket->mutex,
                                           &deadline);
#endif
    }
    if (wait_result == ETIMEDOUT) {
      result = kGuestAddrWaitTimedOut;
      break;
    }
    if (wait_result != 0) {
      result = kGuestAddrWaitError;
      break;
    }
  }
  bucket->waiter_count--;
  pthread_mutex_unlock(&bucket->mutex);
  return result;
}

int GuestAddrWake(GuestAddrWaitTable* table, uint64_t guest_address,
                  bool wake_all) {
  if (table == NULL || !table->initialized) {
    return kGuestAddrWaitError;
  }
  GuestAddrWaitBucket* bucket = GetBucket(table, guest_address, false);
  if (bucket == NULL) {
    return 0;
  }
  pthread_mutex_lock(&bucket->mutex);
  int waiter_count = (int)bucket->waiter_count;
  if (wake_all) {
    pthread_cond_broadcast(&bucket->condition);
  } else if (waiter_count != 0) {
    pthread_cond_signal(&bucket->condition);
  }
  pthread_mutex_unlock(&bucket->mutex);
  return wake_all ? waiter_count : (waiter_count != 0 ? 1 : 0);
}
