// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <chrono>

int main() {
  using namespace std::chrono;
  milliseconds from_ms = duration_cast<milliseconds>(nanoseconds(10000000000000000LL));
  if (from_ms.count() != 10000000000LL) return 1;

  seconds whole = duration_cast<seconds>(milliseconds(1500));
  if (whole.count() != 1) return 2;
  seconds negative = duration_cast<seconds>(milliseconds(-1500));
  if (negative.count() != -1) return 3;

  nanoseconds scaled(milliseconds(3));
  if (scaled.count() != 3000000LL) return 4;
  return 0;
}
