// RUN: -std=c++20

extern void* malloc(unsigned long size);
extern void free(void* ptr);

constexpr int pcode_double_free(void) {
  int* value = (int*)malloc(sizeof(int));
  *value = 42;
  free(value);
  free(value);
  return 42;
}

static_assert(pcode_double_free() == 42,
              "constexpr pcode double free is invalid");
