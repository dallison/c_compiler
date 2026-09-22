// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <typename T>
using EnableIfView = std::enable_if_t<std::is_pointer<T>::value, int>;
template <typename T>
using EnableIfNotView = std::enable_if_t<!std::is_pointer<T>::value, int>;

template <typename T>
struct Span {
  template <typename V, typename = EnableIfNotView<V>>
  explicit Span(V&) {}

  template <typename V, EnableIfView<V> = 0>
  explicit Span(V&) {}

  int value() const { return 1; }
};

int main() {
  int x = 0;
  Span<int> a(x);
  int* p = &x;
  Span<int> b(p);
  return a.value() + b.value() - 2;
}
