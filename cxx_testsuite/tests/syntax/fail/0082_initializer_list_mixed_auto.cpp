// RUN: -std=c++20
// EXPECT: Cannot deduce auto type from mixed braced initializer types

namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;
};
}

void fail_mixed_auto_initializer_list() {
  auto values = {1, 2L};
}
