// RUN: -std=c++17
// EXPECT_EXIT: 0

// A derived class may convert `this` to a private base when calling an
// overloaded member through a member alias (`StorageT<I>::get()`).  A single
// `get` candidate used to succeed via a later derived-to-base conversion;
// two overloads go through ranking, which previously required a public base.

#include <stddef.h>

template <class T, size_t I, class Tag>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  explicit constexpr Storage(T v) : value(v) {}
  constexpr const T& get() const& { return value; }
  constexpr T& get() & { return value; }
};

template <class T>
struct Tag {};

template <class T>
class Tuple : private Storage<T, 0, Tag<T>> {
  template <int I>
  using StorageT = Storage<T, I, Tag<T>>;

 public:
  explicit constexpr Tuple(T v) : Storage<T, 0, Tag<T>>(v) {}

  template <int I>
  constexpr T& get() & {
    return StorageT<I>::get();
  }
};

int main() {
  Tuple<long> t(5l);
  if (t.get<0>() != 5l) {
    return 1;
  }
  Tuple<int> i(7);
  return i.get<0>() == 7 ? 0 : 2;
}
