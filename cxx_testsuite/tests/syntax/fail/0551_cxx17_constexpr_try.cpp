// RUN: -std=c++17
// EXPECT: try-block in a constexpr function requires C++20

constexpr int with_try() {
  try {
    return 1;
  } catch (...) {
    return 0;
  }
}
