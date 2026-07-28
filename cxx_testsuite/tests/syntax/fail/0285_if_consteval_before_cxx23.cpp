// RUN: -std=c++20
// EXPECT: 'if consteval' requires C++23

constexpr int select_context() {
  if consteval {
    return 1;
  } else {
    return 2;
  }
}
