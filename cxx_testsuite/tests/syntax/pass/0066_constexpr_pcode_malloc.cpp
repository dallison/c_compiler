// RUN: -std=c++20

extern void* malloc(unsigned long size);
extern void free(void* ptr);
extern void* realloc(void* ptr, unsigned long size);

constexpr int pcode_malloc_roundtrip(void) {
  int* value = (int*)malloc(sizeof(int));
  *value = 42;
  int result = *value;
  free(value);
  return result;
}

constexpr int pcode_realloc_preserves_values(void) {
  int* values = (int*)malloc(sizeof(int) * 2);
  values[0] = 10;
  values[1] = 20;
  values = (int*)realloc(values, sizeof(int) * 4);
  values[2] = 5;
  values[3] = 7;
  int result = values[0] + values[1] + values[2] + values[3];
  free(values);
  return result;
}

constexpr int pcode_malloc_reuses_freed_heap(void) {
  int* first = (int*)malloc(sizeof(int) * 150000);
  first[0] = 11;
  first[149999] = 12;
  int before = first[0] + first[149999];
  free(first);

  int* second = (int*)malloc(sizeof(int) * 150000);
  second[0] = 7;
  second[149999] = 12;
  int after = second[0] + second[149999];
  free(second);
  return before + after;
}

static_assert(pcode_malloc_roundtrip() == 42,
              "pcode constexpr malloc/free roundtrip");
static_assert(pcode_realloc_preserves_values() == 42,
              "pcode constexpr realloc preserves existing bytes");
static_assert(pcode_malloc_reuses_freed_heap() == 42,
              "pcode constexpr malloc reuses freed heap");
