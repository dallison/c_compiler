// RUN: -std=c++29
// EXPECT: a defaulted postfix increment or decrement operator cannot be a template

struct counter {
  counter& operator++();

  template <typename T>
  counter operator++(T) = default;
};
