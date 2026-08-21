#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#include <davecc/malloc_internal.h>

enum {
  kIterations = 20000,
  kSlots = 256,
};

static uint32_t NextRandom(uint32_t* state) {
  uint32_t value = *state;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  *state = value;
  return value;
}

static clock_t BenchmarkFixedSize(void) {
  clock_t start = clock();
  for (int iteration = 0; iteration < kIterations; ++iteration) {
    unsigned char* pointer = (unsigned char*)malloc(64);
    if (pointer == NULL) {
      return (clock_t)-1;
    }
    pointer[0] = (unsigned char)iteration;
    pointer[63] = (unsigned char)(iteration >> 8);
    free(pointer);
  }
  return clock() - start;
}

static clock_t BenchmarkMixedTrace(void) {
  void* slots[kSlots] = {0};
  uint32_t random = 0x243f6a88u;
  clock_t start = clock();
  for (int iteration = 0; iteration < kIterations; ++iteration) {
    uint32_t value = NextRandom(&random);
    unsigned int index = value % kSlots;
    if (slots[index] != NULL) {
      free(slots[index]);
      slots[index] = NULL;
    } else {
      size_t size = (size_t)((value >> 8) % 4096u) + 1;
      slots[index] = malloc(size);
      if (slots[index] == NULL) {
        return (clock_t)-1;
      }
    }
  }
  clock_t elapsed = clock() - start;
  for (int index = 0; index < kSlots; ++index) {
    free(slots[index]);
  }
  return elapsed;
}

static clock_t BenchmarkFragmentedTrace(void) {
  void* slots[kSlots];
  clock_t start = clock();
  for (int round = 0; round < kIterations / kSlots; ++round) {
    for (int index = 0; index < kSlots; ++index) {
      size_t size = (size_t)((index * 37 + round * 13) % 1024) + 16;
      slots[index] = malloc(size);
      if (slots[index] == NULL) {
        return (clock_t)-1;
      }
    }
    for (int index = 1; index < kSlots; index += 2) {
      free(slots[index]);
    }
    for (int index = 0; index < kSlots; index += 2) {
      free(slots[index]);
    }
  }
  return clock() - start;
}

static int ContendedWorker(void* argument) {
  uintptr_t seed = (uintptr_t)argument;
  for (int iteration = 0; iteration < kIterations / 2; ++iteration) {
    size_t size = 32 + (size_t)((seed + (uintptr_t)iteration) & 63);
    unsigned char* pointer = (unsigned char*)malloc(size);
    if (pointer == NULL) {
      return 1;
    }
    pointer[0] = (unsigned char)iteration;
    pointer[size - 1] = (unsigned char)(iteration >> 8);
    free(pointer);
  }
  return 0;
}

static clock_t BenchmarkContention(void) {
  thrd_t threads[4];
  clock_t start = clock();
  for (int index = 0; index < 4; ++index) {
    if (thrd_create(&threads[index], ContendedWorker,
                    (void*)(uintptr_t)(index * 17)) != thrd_success) {
      return (clock_t)-1;
    }
  }
  for (int index = 0; index < 4; ++index) {
    int result = -1;
    if (thrd_join(threads[index], &result) != thrd_success || result != 0) {
      return (clock_t)-1;
    }
  }
  return clock() - start;
}

int main(void) {
  clock_t fixed = BenchmarkFixedSize();
  clock_t mixed = BenchmarkMixedTrace();
  clock_t fragmented = BenchmarkFragmentedTrace();
  clock_t contention = BenchmarkContention();
  if (fixed < 0 || mixed < 0 || fragmented < 0 || contention < 0) {
    return 1;
  }
  printf("allocator benchmark (%d operations/trace)\n", kIterations);
  printf("fixed-size ticks: %ld\n", (long)fixed);
  printf("mixed-trace ticks: %ld\n", (long)mixed);
  printf("fragmented-trace ticks: %ld\n", (long)fragmented);
  printf("four-thread ticks: %ld\n", (long)contention);
  DaveHeapStats stats;
  __davecc_heap_get_stats(&stats);
  printf("mapped bytes: %zu\n", stats.mapped_bytes);
  printf("peak allocated bytes: %zu\n", stats.peak_allocated_bytes);
  printf("largest free block: %zu\n", stats.largest_free_block);
  printf("bin lookups: %zu\n", stats.bin_lookups);
  if (!__davecc_heap_check()) {
    return 2;
  }
  return 0;
}
