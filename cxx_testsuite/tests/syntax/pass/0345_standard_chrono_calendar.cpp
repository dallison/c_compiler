// RUN: -std=c++20

#include <chrono>
#include <type_traits>

using namespace std::chrono_literals;
using namespace std::chrono;

static_assert(std::is_same_v<typename hh_mm_ss<milliseconds>::precision, milliseconds>);

int main() {
  const year_month_day ymd = 2024y / March / 15d;
  if (!ymd.ok()) {
    return 1;
  }
  if ((2024y / March / last).day() != day(31)) {
    return 2;
  }
  if ((March / Thursday[2]).ok() && (2024y / March / Thursday[last]).ok()) {
    return 0;
  }
  return 3;
}
