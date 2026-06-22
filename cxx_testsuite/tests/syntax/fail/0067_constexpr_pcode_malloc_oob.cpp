// RUN: -std=c++20

extern void* malloc(unsigned long size);
extern void free(void* ptr);

constexpr int pcode_malloc_out_of_bounds_write(void) {
  int* value = (int*)malloc(sizeof(int));
  value[1] = 42;
  free(value);
  return 42;
}

static_assert(pcode_malloc_out_of_bounds_write() == 42,
              "constexpr pcode malloc bounds are checked");
