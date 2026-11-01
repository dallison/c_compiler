// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <stdexcept>
#include <string>

int main() {
  long double finite = std::stold("2.5");
  if (finite < 2.4L || finite > 2.6L) return 1;

  long double large = std::stold("1e1000");
  if (large < 1e100L) return 2;

  bool overflow = false;
  try {
    (void)std::stold("1e10000");
  } catch (const std::out_of_range&) {
    overflow = true;
  }
  if (!overflow) return 3;

  bool underflow = false;
  try {
    (void)std::stold("-1e-10000");
  } catch (const std::out_of_range&) {
    underflow = true;
  }
  if (!underflow) return 4;

  long double exact_zero = std::stold("0e10000");
  if (exact_zero != 0.0L) return 5;

  return 0;
}
