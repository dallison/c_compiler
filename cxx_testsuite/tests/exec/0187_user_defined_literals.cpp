// RUN: -std=c++20
// EXPECT_EXIT: 0

#include "cxx_testsuite/include/pragma_system_udl.hpp"

int operator""_twice(unsigned long long value) {
  return (int)(value * 2);
}

int operator""_raw(const char* text) {
  int count = 0;
  while (text[count] != 0) {
    count++;
  }
  return count;
}

int operator""_fraw(const char* text) {
  int count = 0;
  while (text[count] != 0) {
    count++;
  }
  return count;
}

int operator "" _ch(char value) {
  return value + 1;
}

int operator""_len(const char* text, unsigned long size) {
  return (int)(text[0] + size);
}

template<char... Chars>
constexpr int operator""_pack() {
  return (0 + ... + Chars);
}

constexpr int operator""_prefer(unsigned long long) {
  return 11;
}

template<char... Chars>
constexpr int operator""_prefer() {
  return 22;
}

constexpr int operator""_preferf(long double) {
  return 33;
}

template<char... Chars>
constexpr int operator""_preferf() {
  return 44;
}

static_assert(123_pack == '1' + '2' + '3');
static_assert(0x2a_pack == '0' + 'x' + '2' + 'a');
static_assert(1'23_pack == '1' + '\'' + '2' + '3');
static_assert(1.5e+2_pack == '1' + '.' + '5' + 'e' + '+' + '2');
static_assert(7_prefer == 11);
static_assert(1.0_preferf == 33);

int main() {
  if (21_twice != 42) {
    return 1;
  }
  if (12345_raw != 5) {
    return 2;
  }
  if (1.25_fraw != 4) {
    return 3;
  }
  if ('A'_ch != 'B') {
    return 4;
  }
  if ("abc"_len != 'a' + 3) {
    return 5;
  }
  if (1s != 1001) {
    return 6;
  }
  if (1us != 2001) {
    return 7;
  }
  if ("a" "b"cstr != 'a' + 'b' + 2) {
    return 8;
  }
  if (456_pack != '4' + '5' + '6') {
    return 9;
  }
  if (1'234_raw != 5) {
    return 10;
  }
  return 0;
}
