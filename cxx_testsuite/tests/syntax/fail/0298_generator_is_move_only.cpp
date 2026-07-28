// RUN: -std=c++23
// EXPECT: Use of deleted function generator

#include <generator>

std::generator<int> values() {
  co_yield 1;
}

void invalid() {
  auto first = values();
  std::generator<int> second(first);
}
