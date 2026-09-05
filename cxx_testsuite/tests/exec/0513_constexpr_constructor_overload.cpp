// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cstddef>

template <std::size_t N>
struct Abi {
  static constexpr std::size_t size = N;
};

template <class A>
struct Vec {
  int data[A::size];

  constexpr Vec(const Vec&) = default;

  constexpr Vec(int value) {
    for (std::size_t i = 0; i < A::size; ++i)
      data[i] = value;
  }

  constexpr int operator[](std::size_t i) const { return data[i]; }
};

constexpr Vec<Abi<4>> value(7);
static_assert(value[0] == 7);
static_assert(value[3] == 7);

int main() {
  Vec<Abi<4>> copy(value);
  return copy[0] == 7 && copy[3] == 7 ? 0 : 1;
}
