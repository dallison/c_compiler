// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>

int main() {
  using namespace std::chrono;
  using namespace std::chrono_literals;

  if ((February / last).day() != day(29)) return 1;
  if ((2000y / February / last).day() != day(29)) return 2;
  if ((1900y / February / last).day() != day(28)) return 3;

  if (operator/(2000y / February / last).time_since_epoch() !=
      operator/(2000y / February / 29).time_since_epoch()) {
    return 4;
  }
  if (operator/(1900y / February / last).time_since_epoch() !=
      operator/(1900y / February / 28).time_since_epoch()) {
    return 5;
  }

  if (operator/(1969y / December / Monday[1]).time_since_epoch() !=
      operator/(1969y / December / 1).time_since_epoch()) {
    return 6;
  }
  if (operator/(1969y / December / Monday[last]).time_since_epoch() !=
      operator/(1969y / December / 29).time_since_epoch()) {
    return 7;
  }

  const year_month_day thanksgiving =
      operator/(operator/(2024y / November / Thursday[4]));
  if (thanksgiving.month() != November || thanksgiving.day() != day(28)) {
    return 8;
  }
  return 0;
}
