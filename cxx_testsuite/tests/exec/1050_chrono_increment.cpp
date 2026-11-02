// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>

int main() {
  using std::chrono::day;
  using std::chrono::month;
  using std::chrono::year;
  using std::chrono::weekday;
  using std::chrono::January;
  using std::chrono::December;
  using std::chrono::Monday;
  using std::chrono::Sunday;
  using std::chrono::Saturday;

  month m = December;
  ++m;
  if (m != January) return 1;
  month m2 = January;
  --m2;
  if (m2 != December) return 2;
  month wrapped = December;
  wrapped++;
  if (wrapped != January) return 3;
  month backed = January;
  backed--;
  if (backed != December) return 4;

  weekday wd = Saturday;
  ++wd;
  if (wd != Sunday) return 5;
  weekday back = Sunday;
  --back;
  if (back != Saturday) return 6;
  weekday monday = Monday;
  monday--;
  if (monday != Sunday) return 7;
  weekday chained = Sunday;
  ++(++chained);
  if (chained != weekday(2)) return 8;

  day d(5);
  ++d;
  if (static_cast<unsigned>(d) != 6u) return 9;
  d--;
  if (static_cast<unsigned>(d) != 5u) return 10;

  year y(2024);
  ++y;
  if (static_cast<int>(y) != 2025) return 11;
  y--;
  if (static_cast<int>(y) != 2024) return 12;

  weekday far = Sunday;
  far += std::chrono::days(2147483648LL);
  if (far != weekday(2)) return 13;
  weekday back_far = Sunday;
  back_far += std::chrono::days(-2147483648LL);
  if (back_far != weekday(5)) return 14;

  year summed(-2000000000);
  summed += std::chrono::years(2147483648LL);
  if (static_cast<int>(summed) != 147483648) return 15;

  std::chrono::hh_mm_ss<std::chrono::seconds> min_seconds(
      std::chrono::seconds::min());
  if (!min_seconds.is_negative() || min_seconds.hours().count() != 2562047788015215LL ||
      min_seconds.minutes().count() != 30 || min_seconds.seconds().count() != 8 ||
      min_seconds.subseconds().count() != 0) {
    return 16;
  }
  std::chrono::hh_mm_ss<std::chrono::milliseconds> min_millis(
      std::chrono::milliseconds::min());
  if (!min_millis.is_negative() || min_millis.hours().count() != 2562047788015LL ||
      min_millis.minutes().count() != 12 || min_millis.seconds().count() != 55 ||
      min_millis.subseconds().count() != 808) {
    return 17;
  }
  return 0;
}
