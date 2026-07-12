// Concurrent malloc/free/realloc from multiple guest threads.
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

enum { kThreads = 3, kIters = 60 };

static int worker(void* arg) {
  int id = *(int*)arg;
  uintptr_t checksum = (uintptr_t)id;
  void* p = NULL;

  for (int i = 0; i < kIters; i++) {
    size_t size = (size_t)(16 + ((id + i) % 48));
    if ((i % 4) == 0) {
      p = realloc(p, size);
    } else {
      void* next = malloc(size);
      if (next == NULL) {
        free(p);
        return 10 + id;
      }
      if (p != NULL) {
        free(p);
      }
      p = next;
    }
    if (p == NULL) {
      return 20 + id;
    }
    memset(p, (unsigned char)(id + i), size < 8 ? size : 8);
    checksum += (uintptr_t)((unsigned char*)p)[0];
  }

  free(p);
  return (int)(checksum & 0x7f);
}

int main(void) {
  thrd_t threads[kThreads];
  int ids[kThreads];
  int results[kThreads];

  for (int i = 0; i < kThreads; i++) {
    ids[i] = i + 1;
    results[i] = -1;
    if (thrd_create(&threads[i], worker, &ids[i]) != thrd_success) {
      return 1;
    }
  }

  uintptr_t combined = 0;
  for (int i = 0; i < kThreads; i++) {
    if (thrd_join(threads[i], &results[i]) != thrd_success) {
      return 2 + i;
    }
    if (results[i] <= 0) {
      return 10 + i;
    }
    combined += (uintptr_t)results[i];
  }

  if (combined == 0) {
    return 21;
  }
  return 0;
}
