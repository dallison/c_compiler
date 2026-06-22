// RUN: -std=c++20

extern void* malloc(unsigned long size);

constexpr int pcode_leaks_allocation(void) {
  int* value = (int*)malloc(sizeof(int));
  *value = 42;
  return *value;
}

static_assert(pcode_leaks_allocation() == 42,
              "constexpr pcode allocations must be released");
