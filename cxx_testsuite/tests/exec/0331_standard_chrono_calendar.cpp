// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>
using namespace std::chrono_literals;
using namespace std::chrono;

#if __cpp_lib_chrono != 201907L
#error "unexpected __cpp_lib_chrono value"
#endif

int check_duration_helpers(void) {
  if (floor<seconds>(milliseconds(2500)).count() != 2) {
    return 10;
  }
  if (ceil<seconds>(milliseconds(2500)).count() != 3) {
    return 11;
  }
  if (round<seconds>(milliseconds(3500)).count() != 4) {
    return 12;
  }
  if (abs(seconds(-3)).count() != 3) {
    return 13;
  }
  if (hh_mm_ss<milliseconds>(3h + 15min + 30s + 250ms).hours().count() != 3) {
    return 14;
  }
  if (!hh_mm_ss<milliseconds>(-90s).is_negative()) {
    return 15;
  }
  if (duration_values<unsigned>::min() != 0 ||
      duration_values<unsigned>::max() !=
          std::numeric_limits<unsigned>::max()) {
    return 16;
  }
  return 0;
}

int check_calendar(void) {
  if (!year(2000).is_leap()) {
    return 20;
  }
  if (year(1900).is_leap()) {
    return 21;
  }

  const sys_days epoch = operator/(1970y / January / 1d);
  const sys_days before_epoch = operator/(1969y / December / 31d);
  if ((before_epoch - epoch).count() != -1) {
    return 22;
  }
  if (!(2000y / February / 29d).ok()) {
    return 23;
  }
  if ((1900y / February / 29d).ok()) {
    return 24;
  }

  const year_month_day rolled_date =
      (2024y / January / 31d) + months(1);
  if (rolled_date.month() != February || rolled_date.day() != day(31) ||
      rolled_date.ok()) {
    return 25;
  }

  const weekday thursday_epoch =
      weekday(static_cast<unsigned>((duration_cast<days>(epoch.time_since_epoch()).count() + 4) % 7));
  if (thursday_epoch != Thursday) {
    return 26;
  }
  const weekday runtime_thursday(4);
  const weekday runtime_friday(5);
  if ((runtime_thursday - runtime_friday).count() != 6) {
    return 34;
  }
  if (runtime_thursday[4].weekday().c_encoding() != 4) {
    return 35;
  }
  if (Sunday.c_encoding() != 0 || Sunday.iso_encoding() != 7 ||
      weekday(7).ok()) {
    return 37;
  }
  month advanced = January + months(13);
  month retreated = January - months(1);
  if (advanced != February) return 38;
  if (retreated != December) return 40;
  months month_difference = January - December;
  if (month_difference.count() != 1)
    return static_cast<int>(month_difference.count());
  if (year::min() != year(-32767) || year::max() != year(32767) ||
      -year(2024) != year(-2024)) {
    return 39;
  }
  const sys_days thanksgiving =
      operator/(2024y / November / runtime_thursday[4]);
  if (thanksgiving.time_since_epoch().count() != 20055) {
    return 36;
  }
  if (weekday(static_cast<unsigned>(
          (duration_cast<days>(thanksgiving.time_since_epoch()).count() + 4) % 7))
          .c_encoding() != Thursday.c_encoding()) {
    return 27;
  }
  if ((operator/(thanksgiving)).month() != November) {
    return 28;
  }

  const sys_days last_monday = operator/(2024y / January / Monday[last]);
  if (weekday(static_cast<unsigned>(
          (duration_cast<days>(last_monday.time_since_epoch()).count() + 4) % 7))
          .c_encoding() != Monday.c_encoding()) {
    return 29;
  }

  if (!(operator/(before_epoch)).ok()) {
    return 30;
  }
  if ((operator/(before_epoch)).day() != day(31)) {
    return 31;
  }
  if (local_days_from_sys_days(epoch).time_since_epoch() !=
      epoch.time_since_epoch()) {
    return 32;
  }
  if (sys_days_from_local_days(local_days(days(10))).time_since_epoch().count() != 10) {
    return 33;
  }
  if (!(February / 29).ok()) {
    return 41;
  }
  if ((February / 30).ok()) {
    return 42;
  }
  return 0;
}

int main(void) {
  const int duration_rc = check_duration_helpers();
  if (duration_rc != 0) {
    return duration_rc;
  }
  return check_calendar();
}
