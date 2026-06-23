// RUN: -std=c++20
// EXPECT: Cannot deduce auto type from empty braced initializer

namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;
};
}

void fail_empty_auto_initializer_list() {
  auto values = {};
}
