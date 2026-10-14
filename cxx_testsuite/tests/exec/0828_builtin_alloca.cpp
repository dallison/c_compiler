// alloca storage is aligned, survives calls and nested blocks until the
// function returns, and a function using it is not inlined into a loop.
#include <alloca.h>
#include <stdint.h>
#include <string.h>

static int Sum(const int* values, int count) {
  int total = 0;
  for (int i = 0; i < count; i++) total += values[i];
  return total;
}

static int FillAndSum(int count) {
  int* values = static_cast<int*>(alloca(count * sizeof(int)));
  for (int i = 0; i < count; i++) values[i] = i + 1;
  return Sum(values, count);
}

static int TouchEnds(int bytes) {
  char* p = static_cast<char*>(alloca(bytes));
  p[0] = 1;
  p[bytes - 1] = 2;
  return p[0] + p[bytes - 1];
}

static uintptr_t Misalignment(size_t size) {
  void* p = __builtin_alloca(size);
  memset(p, 0xab, size);
  return reinterpret_cast<uintptr_t>(p) % 16;
}

static int OutlivesBlock(int count) {
  char* kept = nullptr;
  {
    char* inner = static_cast<char*>(alloca(count));
    memset(inner, 'x', count);
    kept = inner;
  }
  char* later = static_cast<char*>(alloca(count));
  memset(later, 'y', count);
  int same = 0;
  for (int i = 0; i < count; i++) same += kept[i] == 'x';
  return same;
}

template <class T>
T TemplatedSum(int count) {
  T* values = static_cast<T*>(alloca(sizeof(T) * count));
  for (int i = 0; i < count; i++) values[i] = static_cast<T>(i);
  T total = 0;
  for (int i = 0; i < count; i++) total += values[i];
  return total;
}

int main() {
  if (FillAndSum(10) != 55) return 1;
  int total = 0;
  for (int i = 0; i < 100; i++) total += FillAndSum(100) == 5050;
  // 200 MB if the allocations accumulated in main's frame.
  for (int i = 0; i < 50000; i++) total += TouchEnds(4096) == 3;
  if (total != 50100) return 2;
  for (size_t size = 1; size < 64; size += 7) {
    if (Misalignment(size) != 0) return 3;
  }
  if (OutlivesBlock(33) != 33) return 4;
  if (TemplatedSum<long>(5) != 10) return 5;
  if (TemplatedSum<double>(4) != 6.0) return 6;
  return 0;
}
