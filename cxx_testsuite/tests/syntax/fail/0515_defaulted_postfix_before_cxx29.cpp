// RUN: -std=c++26
// EXPECT: defaulted postfix increment and decrement operators require C++29

struct counter {
  counter& operator++();
  counter operator++(int) = default;
};
