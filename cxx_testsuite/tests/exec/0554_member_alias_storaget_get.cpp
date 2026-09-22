// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <stddef.h>

template <typename T, size_t I>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  constexpr T& get() & { return value; }
  constexpr T&& get() && { return static_cast<T&&>(value); }
};

template <typename T>
struct Impl : Storage<T, 0> {};

template <typename T>
struct Tuple : Impl<T> {
  template <int I>
  using StorageT = Storage<T, I>;

  template <int I>
  constexpr T& get() & {
    return StorageT<I>::get();
  }

  template <int I>
  constexpr T&& get() && {
    return static_cast<Tuple&&>(*this).StorageT<I>::get();
  }
};

int main() {
  Tuple<int> t;
  t.get<0>() = 7;
  int moved = static_cast<Tuple<int>&&>(t).get<0>();
  return moved == 7 ? 0 : 1;
}
