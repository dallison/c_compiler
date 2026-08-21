#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <davecc/malloc_internal.h>

enum {
  kSlots = 96,
  kIterations = 6000,
};

typedef struct {
  unsigned char* pointer;
  size_t size;
  unsigned char tag;
} Allocation;

static uint32_t NextRandom(uint32_t* state) {
  uint32_t value = *state;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  *state = value;
  return value;
}

static int CheckAllocation(const Allocation* allocation) {
  if (allocation->pointer == NULL) {
    return allocation->size == 0;
  }
  if (((uintptr_t)allocation->pointer & 15) != 0) {
    return 0;
  }
  if (allocation->size != 0 &&
      (allocation->pointer[0] != allocation->tag ||
       allocation->pointer[allocation->size - 1] != allocation->tag)) {
    return 0;
  }
  return 1;
}

static void MarkAllocation(Allocation* allocation) {
  if (allocation->size == 0) {
    return;
  }
  allocation->pointer[0] = allocation->tag;
  allocation->pointer[allocation->size - 1] = allocation->tag;
}

static int TestBasicSemantics(void) {
  free(NULL);

  void* zero = malloc(0);
  if (zero == NULL || ((uintptr_t)zero & 15) != 0) {
    return 1;
  }
  free(zero);

  unsigned char* value = (unsigned char*)realloc(NULL, 37);
  if (value == NULL || ((uintptr_t)value & 15) != 0) {
    return 2;
  }
  memset(value, 0x5a, 37);
  value = (unsigned char*)realloc(value, 211);
  if (value == NULL || ((uintptr_t)value & 15) != 0) {
    return 3;
  }
  for (size_t index = 0; index < 37; ++index) {
    if (value[index] != 0x5a) {
      return 4;
    }
  }
  value = (unsigned char*)realloc(value, 19);
  if (value == NULL) {
    return 5;
  }
  for (size_t index = 0; index < 19; ++index) {
    if (value[index] != 0x5a) {
      return 6;
    }
  }
  free(value);

  if (calloc((size_t)-1, 2) != NULL) {
    return 7;
  }
  unsigned int* cleared = (unsigned int*)calloc(64, sizeof(unsigned int));
  if (cleared == NULL) {
    return 8;
  }
  for (size_t index = 0; index < 64; ++index) {
    if (cleared[index] != 0) {
      return 9;
    }
  }
  free(cleared);

  unsigned char* aligned = (unsigned char*)aligned_alloc(64, 256);
  if (aligned == NULL || ((uintptr_t)aligned & 63) != 0) {
    return 10;
  }
  aligned[0] = 0x3c;
  aligned[255] = 0xc3;
  free(aligned);
  if (!__davecc_heap_check()) {
    return 15;
  }

  value = (unsigned char*)malloc(331);
  if (value == NULL) {
    return 11;
  }
  value[0] = 23;
  value[330] = 23;
  if (value[0] != 23 || value[330] != 23) {
    return 16;
  }
#if defined(__x86_64__)
  if (__davecc_malloc_cache_class(value) != 336) {
    return 13;
  }
#endif
  value = (unsigned char*)realloc(value, 705);
  if (value == NULL || value[0] != 23 || value[330] != 23) {
    return 12;
  }
  free(value);
  return 0;
}

static int TestDeterministicTrace(void) {
  Allocation slots[kSlots];
  memset(slots, 0, sizeof(slots));
  uint32_t random = 0x9e3779b9u;

  for (int iteration = 0; iteration < kIterations; ++iteration) {
    uint32_t value = NextRandom(&random);
    Allocation* slot = &slots[value % kSlots];
    if (!CheckAllocation(slot)) {
      return 10;
    }

    if (slot->pointer != NULL && (value & 3u) == 0) {
      free(slot->pointer);
      memset(slot, 0, sizeof(*slot));
      continue;
    }

    size_t new_size = (size_t)((value >> 8) % 2048u) + 1;
    if (slot->pointer == NULL) {
      slot->pointer = (unsigned char*)malloc(new_size);
      if (slot->pointer == NULL) {
        return 11;
      }
      slot->size = new_size;
      slot->tag = (unsigned char)(value | 1u);
    } else {
      size_t old_size = slot->size;
      unsigned char old_tag = slot->tag;
      unsigned char* replacement =
          (unsigned char*)realloc(slot->pointer, new_size);
      if (replacement == NULL) {
        return 12;
      }
      slot->pointer = replacement;
      slot->size = new_size;
      if (old_size != 0 && slot->pointer[0] != old_tag) {
        return 13;
      }
      if (old_size != 0 && new_size >= old_size &&
          slot->pointer[old_size - 1] != old_tag) {
        return 16;
      }
      slot->tag = (unsigned char)((value >> 16) | 1u);
    }
    MarkAllocation(slot);
  }

  for (int index = 0; index < kSlots; ++index) {
    if (!CheckAllocation(&slots[index])) {
      return 14;
    }
    free(slots[index].pointer);
  }
  if (!__davecc_heap_check()) {
    return 15;
  }
  return 0;
}

static int TestCoalescing(void) {
  void* blocks[128];
  for (int index = 0; index < 128; ++index) {
    blocks[index] = malloc(1024);
    if (blocks[index] == NULL) {
      return 20;
    }
  }
  for (int index = 1; index < 128; index += 2) {
    free(blocks[index]);
  }
  for (int index = 0; index < 128; index += 2) {
    free(blocks[index]);
  }
  void* combined = malloc(96 * 1024);
  if (combined == NULL) {
    return 21;
  }
  free(combined);

  void* class_blocks[64];
  for (int index = 0; index < 64; ++index) {
    class_blocks[index] = malloc((size_t)(index + 1) * 16);
    if (class_blocks[index] == NULL) {
      return 22;
    }
  }
  for (int index = 0; index < 64; ++index) {
    free(class_blocks[index]);
  }
  combined = malloc(700 * 1024);
  if (combined == NULL) {
    return 23;
  }
  free(combined);
  if (!__davecc_heap_check()) {
    return 24;
  }
  return 0;
}

int main(void) {
  int result = TestBasicSemantics();
  if (result != 0) {
    return result;
  }
  result = TestDeterministicTrace();
  if (result != 0) {
    return result;
  }
  return TestCoalescing();
}
