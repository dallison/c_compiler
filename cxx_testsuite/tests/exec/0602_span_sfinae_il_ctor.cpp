// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cstddef>
#include <initializer_list>
#include <type_traits>

template <class T>
struct Span {
  T* ptr;
  size_t len;

  template <class U>
  using EnableIfConst =
      typename std::enable_if<std::is_const<T>::value, U>::type;

  Span() : ptr(nullptr), len(0) {}
  Span(T* p, size_t n) : ptr(p), len(n) {}

  template <class LazyT = T, class = EnableIfConst<LazyT>>
  Span(std::initializer_list<typename std::remove_const<T>::type> v)
      : Span(v.begin(), v.size()) {}
};

int main() {
  Span<int> empty;
  int a = 3;
  Span<int> from_ptr(&a, 1);
  const int c[2] = {4, 5};
  Span<const int> from_array(c, 2);
  Span<const int> from_il({6, 7});
  return empty.ptr == nullptr && from_ptr.len == 1 && from_ptr.ptr[0] == 3 &&
                 from_array.len == 2 && from_il.len == 2 &&
                 from_il.ptr[0] == 6 && from_il.ptr[1] == 7
             ? 0
             : 1;
}
