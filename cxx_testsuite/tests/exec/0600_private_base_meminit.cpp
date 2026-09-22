// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

struct in_place_t {
  explicit in_place_t() = default;
};
constexpr in_place_t in_place{};

template <class T, int I>
struct Storage {
  T value;
  Storage() : value() {}
  template <class V>
  explicit Storage(in_place_t, V&& v) : value(static_cast<V&&>(v)) {}
};

template <int... I>
struct IndexSeq {};

template <class D, class I>
struct Impl;

template <class... Ts>
struct Tag {};

template <class... Ts, int... I>
struct Impl<Tag<Ts...>, IndexSeq<I...>> : Storage<Ts, I>... {
  Impl() = default;
  template <class... Vs>
  explicit Impl(in_place_t, Vs&&... args)
      : Storage<Ts, I>(in_place, static_cast<Vs&&>(args))... {}
};

template <class... Ts>
class Derived : private Impl<Tag<Ts...>, IndexSeq<0, 1>> {
 public:
  explicit Derived(const Ts&... base) : Derived::Impl(in_place, base...) {}
  int first() const {
    return static_cast<const Storage<int, 0>*>(
               static_cast<const Impl<Tag<Ts...>, IndexSeq<0, 1>>*>(this))
        ->value;
  }
  unsigned second() const {
    return static_cast<const Storage<unsigned, 1>*>(
               static_cast<const Impl<Tag<Ts...>, IndexSeq<0, 1>>*>(this))
        ->value;
  }
};

int main() {
  Derived<int, unsigned> x(5, 7u);
  return x.first() == 5 && x.second() == 7 ? 0 : 1;
}
