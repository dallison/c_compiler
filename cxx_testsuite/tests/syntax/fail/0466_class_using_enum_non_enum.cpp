// RUN: -std=c++20
// EXPECT: is not an enumeration type

struct NotAnEnum {};

struct Invalid {
  using enum NotAnEnum;
};
