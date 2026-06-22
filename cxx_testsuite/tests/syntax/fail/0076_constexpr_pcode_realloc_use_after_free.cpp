// RUN: -std=c++20

extern void* malloc(unsigned long size);
extern void free(void* ptr);
extern void* realloc(void* ptr, unsigned long size);

constexpr int pcode_realloc_use_after_free(void) {
  int* original = (int*)malloc(sizeof(int) * 2);
  original[0] = 10;
  original[1] = 32;
  int* resized = (int*)realloc(original, sizeof(int) * 8);
  resized[2] = 1;
  int result = original[0] + resized[1] + resized[2];
  free(resized);
  return result;
}

static_assert(pcode_realloc_use_after_free() == 43,
              "constexpr pcode use after realloc is invalid");
