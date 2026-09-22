// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <chrono>
#include <strings.h>

int main() {
  using namespace std::chrono;
  milliseconds ms(1500);
  duration<long double, std::nano> ns(ms);
  if (ns.count() < 1499999999.0L || ns.count() > 1500000001.0L) {
    return 1;
  }
  seconds s = duration_cast<seconds>(ms);
  if (s.count() != 1) {
    return 2;
  }
  if (ffsll(0) != 0) {
    return 3;
  }
  if (ffsll(8LL) != 4) {
    return 4;
  }
  if (ffsll(1LL << 40) != 41) {
    return 5;
  }
  return 0;
}
