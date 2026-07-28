// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <format>

int main() {
  if (std::format("{} {} {}", 42, -7, true) != "42 -7 true") return 1;
  if (std::format("{1} {0} {{ok}}", "left", "right") !=
      "right left {ok}") return 2;
  if (std::format("{:#x} {:#X} {:#o} {:#b}", 42, 42, 42, 10) !=
      "0x2a 0X2A 052 0b1010") return 3;
  if (std::format("{:+d} {: d} {:05d}", 7, 7, -12) != "+7  7 -0012")
    return 4;
  if (std::format("{:*^7}", "x") != "***x***") return 5;
  if (std::format("{:.3}", "abcdef") != "abc") return 6;
  if (std::format("{:.2f}", 3.5) != "3.50") return 7;
  if (std::format("{} {:d}", 'A', 'A') != "A 65") return 8;
  int value = 0;
  void* address = &value;
  std::string pointer = std::format("{}", address);
  if (pointer.size() < 3 || pointer[0] != '0' || pointer[1] != 'x') return 9;
  return 0;
}
