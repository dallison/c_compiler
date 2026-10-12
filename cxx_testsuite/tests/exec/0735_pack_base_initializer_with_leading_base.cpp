// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <tuple>
#include <utility>

struct Marker {};

template <class T, int I>
struct Storage {
  template <class V>
  explicit Storage(V&& value) : value_(static_cast<T>(value)) {}

  T value_;
};

template <class Types, class Indices>
struct Impl;

template <class... Ts, int... I>
struct Impl<std::tuple<Ts...>, std::integer_sequence<int, I...>>
    : Marker, Storage<Ts, I>... {
  template <class... Vs>
  explicit Impl(Vs&&... values)
      : Storage<Ts, I>(std::forward<Vs>(values))... {}
};

int main() {
  Impl<std::tuple<int, long, char>,
       std::integer_sequence<int, 0, 1, 2>>
      values(4, 5L, '6');
  (void)values;
  return 0;
}
