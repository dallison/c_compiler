// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <limits>
#include <sstream>

int main() {
  std::istringstream too_wide("4294967296");
  unsigned int value = 7;
  too_wide >> value;
  if (!too_wide.fail()) return 1;
  if (value != std::numeric_limits<unsigned int>::max()) return 2;

  std::istringstream too_short("99999");
  short narrow = 3;
  too_short >> narrow;
  if (!too_short.fail()) return 3;
  if (narrow != std::numeric_limits<short>::max()) return 4;

  std::istringstream too_negative("-99999");
  short negative = 3;
  too_negative >> negative;
  if (!too_negative.fail()) return 5;
  if (negative != std::numeric_limits<short>::min()) return 6;

  std::istringstream fits("12 -34");
  int first = 0;
  int second = 0;
  fits >> first >> second;
  if (!fits || first != 12 || second != -34) return 7;

  std::istringstream full("18446744073709551615");
  unsigned long long all_bits = 0;
  full >> all_bits;
  if (!full || all_bits != 18446744073709551615ULL) return 8;

  std::istringstream past("18446744073709551616");
  unsigned long long saturated = 9;
  past >> saturated;
  if (!past.fail()) return 9;
  if (saturated != 18446744073709551615ULL) return 10;

  std::istringstream huge("9223372036854775808");
  long long signed_huge = 4;
  huge >> signed_huge;
  if (!huge.fail()) return 11;
  if (signed_huge != std::numeric_limits<long long>::max()) return 12;
  return 0;
}
