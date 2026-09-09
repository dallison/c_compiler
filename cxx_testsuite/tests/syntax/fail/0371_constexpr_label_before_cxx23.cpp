// RUN: -std=c++20
// EXPECT: label in a constexpr function requires C++23

constexpr int labeled() {
label:
  return 7;
}

static_assert(labeled() == 7);
