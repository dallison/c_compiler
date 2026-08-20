// RUN: -std=c++29
// EXPECT: only postfix increment and decrement operators can be defaulted

struct counter {
  counter& operator++() = default;
};
