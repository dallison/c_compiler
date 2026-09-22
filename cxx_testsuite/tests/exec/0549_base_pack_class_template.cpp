// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class T, int I>
struct Slot {
  using type = T;
  static constexpr int index = I;
};

template <class... Ts>
struct Pack {};

template <int... Is>
struct Idx {};

template <class P, class I>
struct Impl;

template <class... Ts, int... Is>
struct Impl<Pack<Ts...>, Idx<Is...>> : Slot<Ts, Is>... {};

int main() {
  using H = Impl<Pack<int, char>, Idx<0, 1>>;
  static_assert(std::is_base_of<Slot<int, 0>, H>::value);
  static_assert(std::is_base_of<Slot<char, 1>, H>::value);
  static_assert(!std::is_base_of<Slot<int, 1>, H>::value);
  return 0;
}
