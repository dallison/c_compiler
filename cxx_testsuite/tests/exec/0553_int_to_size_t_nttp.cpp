// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <stddef.h>

template <size_t I>
struct Storage {
  static constexpr size_t get() { return I; }
};

template <int I>
struct Wrap : Storage<I> {};

int main() { return Wrap<3>::get() == 3 ? 0 : 1; }
