// RUN: -std=c++11

#include <chrono>

static_assert(std::chrono::seconds(2).count() == 2, "duration");
static_assert(std::chrono::duration_values<unsigned>::min() == 0,
              "unsigned duration minimum");

int main() {
  std::chrono::milliseconds value(1500);
  return std::chrono::duration_cast<std::chrono::seconds>(value).count() != 1;
}
