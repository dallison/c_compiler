// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <stddef.h>

struct A {
  int x;
  int y;
};

template <class T>
T make() {
  return T();
}

int take(int* p) { return p == nullptr ? 0 : 1; }

int main() {
  int A::*value_init = make<int A::*>();
  int A::*literal = 0;
  if (value_init != nullptr || literal != nullptr) {
    return 1;
  }
  int A::*member = &A::y;
  if (member == nullptr) return 2;
  int* pointer = make<int*>();
  if (pointer != nullptr) return 3;
  if (take(0) + take((0)) + take(0L) + take(0u) + take(0x0) + take(NULL) != 0) {
    return 4;
  }
  void* untyped = 0;
  return untyped == nullptr ? 0 : 5;
}
