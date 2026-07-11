// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>
#include <ratio>
#include <type_traits>

static_assert(std::ratio<2, 4>::num == 1, "ratio num normalization");
static_assert(std::ratio<2, 4>::den == 2, "ratio den normalization");
static_assert(std::ratio_equal<std::ratio_add<std::ratio<1, 3>,
                                              std::ratio<1, 6> >,
                               std::ratio<1, 2> >::value,
              "ratio_add");
static_assert(std::ratio_less<std::milli, std::ratio<1> >::value,
              "ratio_less");

using namespace std::chrono;
using namespace std::chrono_literals;

int main() {
  seconds s(2);
  milliseconds ms = duration_cast<milliseconds>(s);
  if (ms.count() != 2000) {
    return 1;
  }

  if ((500ms + 2s).count() != 2500) {
    return 2;
  }

  if (duration_cast<seconds>(2500ms).count() != 2) {
    return 3;
  }

  if (1min != 60s) {
    return 4;
  }

  if (1us != 1000ns) {
    return 5;
  }

  time_point<system_clock, seconds> epoch(seconds(10));
  time_point<system_clock, milliseconds> later = epoch + 500ms;
  if ((later - epoch).count() != 500) {
    return 6;
  }

  if (time_point_cast<seconds>(later).time_since_epoch().count() != 10) {
    return 7;
  }

  system_clock::time_point from = system_clock::from_time_t(12);
  if (system_clock::to_time_t(from) != 12) {
    return 8;
  }

  return 0;
}
