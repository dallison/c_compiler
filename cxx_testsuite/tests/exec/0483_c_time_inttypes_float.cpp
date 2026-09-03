// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cfloat>
#include <cinttypes>
#include <cstring>
#include <ctime>

int main() {
  std::time_t epoch = 0;
  std::tm value;
  if (std::gmtime_r(&epoch, &value) == 0 || value.tm_year != 70 ||
      value.tm_mon != 0 || value.tm_mday != 1 || value.tm_wday != 4) {
    return 1;
  }

  char formatted[64];
  if (std::strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S",
                    &value) != 19 ||
      std::strcmp(formatted, "1970-01-01 00:00:00") != 0) {
    return 2;
  }

  std::tm parsed = {};
  char* end = std::strptime("2026-09-02 23:45:06!", "%Y-%m-%d %H:%M:%S",
                            &parsed);
  if (end == 0 || *end != '!' || parsed.tm_year != 126 ||
      parsed.tm_mon != 8 || parsed.tm_mday != 2 ||
      parsed.tm_hour != 23 || parsed.tm_min != 45 || parsed.tm_sec != 6) {
    return 3;
  }

  std::time_t sample = 123456;
  std::tm local;
  if (std::localtime_r(&sample, &local) == 0 ||
      std::mktime(&local) != sample) return 4;
  if (std::difftime(20, 3) != 17.0) return 5;

  char* integer_end = 0;
  if (std::strtoimax("-123x", &integer_end, 10) != -123 ||
      integer_end == 0 || *integer_end != 'x') return 6;
  if (std::strtoumax("ff", &integer_end, 16) != 255 ||
      integer_end == 0 || *integer_end != '\0') return 7;
  std::imaxdiv_t division = std::imaxdiv(17, 5);
  if (division.quot != 3 || division.rem != 2 ||
      std::imaxabs(-9) != 9) return 8;

  if (FLT_RADIX != 2 || FLT_ROUNDS != 1 || FLT_DECIMAL_DIG < 9 ||
      DBL_DECIMAL_DIG < 17 || FLT_TRUE_MIN >= FLT_MIN) return 9;
  return 0;
}
