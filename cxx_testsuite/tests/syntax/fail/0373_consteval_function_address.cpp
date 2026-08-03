// RUN: -std=c++20
// EXPECT: immediate function may only be named

consteval int identity(int value) {
  return value;
}

auto invalid = &identity;
