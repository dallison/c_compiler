// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cstdint>
#include <cstdio>
#include <climits>

int main() {
  unsigned hex = 0;
  if (std::sscanf("10", "%x", &hex) != 1 || hex != 0x10u) return 1;
  if (std::sscanf("A0", "%x", &hex) != 1 || hex != 0xA0u) return 2;
  if (std::sscanf("0x10", "%x", &hex) != 1 || hex != 0x10u) return 3;
  if (std::sscanf("a", "%x", &hex) != 1 || hex != 0xAu) return 4;
  if (std::sscanf("0", "%x", &hex) != 1 || hex != 0u) return 5;

  void* p = 0;
  if (std::sscanf("A0", "%p", &p) != 1 || p != (void*)0xA0) return 6;
  if (std::sscanf("10", "%p", &p) != 1 || p != (void*)0x10) return 7;

  int n = 7;
  if (std::sscanf("2147483648", "%d", &n) != 1 || n != INT_MAX) return 8;
  if (std::sscanf("-2147483649", "%d", &n) != 1 || n != INT_MIN) return 9;
  if (std::sscanf("-42", "%d", &n) != 1 || n != -42) return 10;
  if (std::sscanf("-2147483648", "%d", &n) != 1 || n != INT_MIN) return 11;

  unsigned u = 1;
  if (std::sscanf("4294967296", "%u", &u) != 1 || u != UINT_MAX) return 12;
  if (std::sscanf("-1", "%u", &u) != 1 || u != UINT_MAX) return 13;

  short sh = 1;
  if (std::sscanf("99999", "%hd", &sh) != 1 || sh != SHRT_MAX) return 14;
  if (std::sscanf("-32768", "%hd", &sh) != 1 || sh != SHRT_MIN) return 15;

  unsigned long long big = 0;
  if (std::sscanf("18446744073709551615", "%llu", &big) != 1 ||
      big != ULLONG_MAX) {
    return 16;
  }
  if (std::sscanf("18446744073709551616", "%llu", &big) != 1 ||
      big != ULLONG_MAX) {
    return 17;
  }

  long long wide = 1;
  if (std::sscanf("-9223372036854775808", "%lld", &wide) != 1 ||
      wide != LLONG_MIN) {
    return 18;
  }
  if (std::sscanf("9223372036854775808", "%lld", &wide) != 1 ||
      wide != LLONG_MAX) {
    return 19;
  }

  int oct = 0;
  if (std::sscanf("010", "%i", &oct) != 1 || oct != 8) return 20;
  int hex_i = 0;
  if (std::sscanf("0x10", "%i", &hex_i) != 1 || hex_i != 16) return 21;
  int dec = 0;
  if (std::sscanf("100", "%d", &dec) != 1 || dec != 100) return 22;

  int counted = 0;
  int consumed = -1;
  if (std::sscanf("42", "%d%n", &counted, &consumed) != 1 || counted != 42 ||
      consumed != 2) {
    return 23;
  }
  return 0;
}
