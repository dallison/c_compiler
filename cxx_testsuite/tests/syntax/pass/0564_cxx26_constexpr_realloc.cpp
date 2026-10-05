// RUN: -std=c++26

// A cast from void* sends a C++26 evaluation through the AST evaluator, which
// must model realloc as well as malloc and free.
extern "C" void* malloc(unsigned long size);
extern "C" void free(void* ptr);
extern "C" void* realloc(void* ptr, unsigned long size);

constexpr int realloc_grows() {
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

constexpr int realloc_shrinks() {
  int* values = (int*)malloc(sizeof(int) * 4);
  values[0] = 3;
  values[1] = 4;
  values = (int*)realloc(values, sizeof(int));
  int result = values[0];
  free(values);
  return result;
}

constexpr int realloc_from_null() {
  int* values = (int*)realloc(nullptr, sizeof(int));
  *values = 9;
  int result = *values;
  free(values);
  return result;
}

static_assert(realloc_grows() == 42);
static_assert(realloc_shrinks() == 3);
static_assert(realloc_from_null() == 9);
