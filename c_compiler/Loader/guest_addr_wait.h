#ifndef guest_addr_wait_h
#define guest_addr_wait_h

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GUEST_ADDR_WAIT_BUCKETS 64

typedef bool (*GuestAddrReadFn)(void* context, uint64_t guest_address,
                                void* value, size_t size);

typedef struct GuestAddrWaitBucket GuestAddrWaitBucket;

typedef struct GuestAddrWaitTable {
  pthread_mutex_t mutex;
  GuestAddrWaitBucket* buckets[GUEST_ADDR_WAIT_BUCKETS];
  bool initialized;
  bool shutting_down;
} GuestAddrWaitTable;

enum {
  kGuestAddrWaitSuccess = 0,
  kGuestAddrWaitTimedOut = 1,
  kGuestAddrWaitError = -1,
};

bool GuestAddrWaitTableInit(GuestAddrWaitTable* table);
void GuestAddrWaitTableShutdown(GuestAddrWaitTable* table);
void GuestAddrWaitTableDestruct(GuestAddrWaitTable* table);

int GuestAddrWait(GuestAddrWaitTable* table, GuestAddrReadFn read,
                  void* context, uint64_t guest_address,
                  const void* expected, size_t size, int64_t timeout_us);
int GuestAddrWake(GuestAddrWaitTable* table, uint64_t guest_address,
                  bool wake_all);

#endif
