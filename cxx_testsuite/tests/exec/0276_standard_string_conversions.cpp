// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <string>
#include <stdexcept>

int main() {
  if (std::to_string(-123) != "-123" ||
      std::to_string(42U) != "42" ||
      std::to_string(-123456789L) != "-123456789" ||
      std::to_string(123456789ULL) != "123456789" ||
      std::to_string(1.5) != "1.500000" ||
      std::to_string(static_cast<long double>(1.25)) != "1.250000") {
    return 1;
  }

  std::size_t pos = 0;
  if (std::stoi("123tail", &pos) != 123 || pos != 3) {
    return 2;
  }
  if (std::stol("-123456") != -123456L ||
      std::stoll("-1234567890123") != -1234567890123LL ||
      std::stoul("ff", nullptr, 16) != 255UL ||
      std::stoull("1234567890123") != 1234567890123ULL ||
      std::stoi("  0x20tail", &pos, 0) != 32 || pos != 6) {
    return 3;
  }

  double d = std::stod("2.5rest", &pos);
  if (d < 2.49 || d > 2.51 || pos != 3) {
    return 4;
  }
  float f = std::stof("-1.25");
  if (f > -1.24f || f < -1.26f) {
    return 5;
  }
  long double ld = std::stold("3.25rest", &pos);
  if (ld < static_cast<long double>(3.24) ||
      ld > static_cast<long double>(3.26) || pos != 4) {
    return 8;
  }

  bool invalid = false;
  try {
    (void)std::stoi("not-a-number");
  } catch (const std::invalid_argument&) {
    invalid = true;
  }
  if (!invalid) {
    return 6;
  }

  bool range = false;
  try {
    (void)std::stoi("2147483648");
  } catch (const std::out_of_range&) {
    range = true;
  }
  if (!range) {
    return 7;
  }

  range = false;
  try {
    (void)std::stoll("9223372036854775808");
  } catch (const std::out_of_range&) {
    range = true;
  }
  if (!range) {
    return 9;
  }

  range = false;
  try {
    (void)std::stoull("18446744073709551616");
  } catch (const std::out_of_range&) {
    range = true;
  }
  return range ? 0 : 10;
}
