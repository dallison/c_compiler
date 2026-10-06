// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT_EXIT: 0

// The p-code constant evaluator's heap for a real target: a pointer argument
// fills a full width slot even where pointers are 32 bits, so the size_t that
// follows it in realloc and placement new must be read after that slot.
#include <memory>
#include <new>
#include <stdlib.h>

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

struct P {
  int a;
  int b;
  constexpr P(int x, int y) : a(x), b(y) {}
};

constexpr int placement_constructed() {
  P* storage = std::allocator<P>().allocate(2);
  P* first = std::construct_at(storage, 1, 2);
  P* second = std::construct_at(storage + 1, 30, 40);
  int result = first->a + first->b + second->a + second->b;
  std::allocator<P>().deallocate(storage, 2);
  return result;
}

constexpr int new_and_delete() {
  P* p = new P(5, 6);
  int* array = new int[3]{7, 8, 9};
  int result = p->a * p->b + array[0] + array[1] + array[2];
  delete p;
  delete[] array;
  return result;
}

static_assert(realloc_grows() == 42);
static_assert(realloc_shrinks() == 3);
static_assert(placement_constructed() == 73);
static_assert(new_and_delete() == 54);

int main() {
  if (realloc_grows() != 42) return 1;
  if (realloc_shrinks() != 3) return 2;
  if (placement_constructed() != 73) return 3;
  if (new_and_delete() != 54) return 4;
  return 0;
}
