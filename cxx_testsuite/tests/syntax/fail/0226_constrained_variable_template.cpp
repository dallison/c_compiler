// RUN: -std=c++20
// EXPECT: constraints not satisfied

template <typename T>
concept Never = false;

template <typename T>
  requires Never<T>
constexpr int rejected_value = 1;

int value = rejected_value<int>;
