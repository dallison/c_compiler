// RUN: -target 65c02 -std=c++20

#include <iterator>

int use_counted_iterator(int* values) {
  std::counted_iterator<int*> current(values, 3L);
  int sum = 0;
  while (current != std::default_sentinel) {
    sum += *current;
    ++current;
  }
  return sum;
}

int main() {}
