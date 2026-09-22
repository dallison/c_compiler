// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <stddef.h>

template <typename T, size_t I>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  explicit constexpr Storage(T v) : value(v) {}
  constexpr T& get() & { return value; }
  constexpr const T& get() const& { return value; }
};

template <typename T0, typename T1>
struct Impl : Storage<T0, 0>, Storage<T1, 1> {
  Impl(T0 a, T1 b) : Storage<T0, 0>(a), Storage<T1, 1>(b) {}
};

template <typename T0, typename T1>
struct Tuple : Impl<T0, T1> {
  Tuple(T0 a, T1 b) : Impl<T0, T1>(a, b) {}

  template <int I>
  using StorageT = Storage<T0, I>;

  template <int I>
  constexpr T0& get() & {
    return StorageT<I>::get();
  }

  template <int I>
  constexpr const T0& get() const& {
    return StorageT<I>::get();
  }
};

int main() {
  Tuple<int, unsigned> t(5, 7u);
  if (t.get<0>() != 5) {
    return 1;
  }
  t.get<0>() = 9;
  const Tuple<int, unsigned>& c = t;
  return c.get<0>() == 9 ? 0 : 2;
}
