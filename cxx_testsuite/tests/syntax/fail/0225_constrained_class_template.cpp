// RUN: -std=c++20
// EXPECT: constraints not satisfied

template <typename T>
concept Never = false;

template <typename T>
  requires Never<T>
struct Rejected {
};

Rejected<int> rejected;
