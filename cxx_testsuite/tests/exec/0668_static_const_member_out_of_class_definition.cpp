// RUN: -std=c++17
// EXPECT_EXIT: 0

// A `static const` integral or enum member with an in-class initializer is
// only declared in the class; `const T C::m;` at namespace scope defines it
// and must carry the in-class value.  Binding a reference odr-uses it (as
// gmock's `vector(n, kUnused)` does).

#include <stddef.h>

#include <vector>

enum E { kA = 4 };

struct S {
  static const size_t kUnused = static_cast<size_t>(-1);
  static const int b = 3;
  static const E e = kA;
  static constexpr int c = 9;
  static const int d;
};

const size_t S::kUnused;
const int S::b;
const E S::e;
constexpr int S::c;
const int S::d = 11;

template <class T>
struct TS {
  static const int v = sizeof(T);
};
template <class T>
const int TS<T>::v;

const int* Address(const int& r) { return &r; }

int main() {
  std::vector<size_t> v(2, S::kUnused);
  if (v[1] != static_cast<size_t>(-1)) {
    return 1;
  }
  if (*Address(S::b) != 3 || *Address(S::c) != 9 || *Address(S::d) != 11) {
    return 2;
  }
  const E& er = S::e;
  if (er != kA) {
    return 3;
  }
  if (*Address(TS<double>::v) != 8) {
    return 4;
  }
  int arr[S::b];
  return sizeof(arr) == 3 * sizeof(int) ? 0 : 5;
}
