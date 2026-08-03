// RUN: -std=c++20

#include <ranges>

double values[13];

int main(void) {
  double count = static_cast<double>(std::ranges::size(values));
  return count == 13.0 ? 0 : 1;
}
