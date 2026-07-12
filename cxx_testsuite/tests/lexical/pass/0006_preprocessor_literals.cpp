// RUN: -std=c++14
#include <cxx_lexical_header.hpp>

constexpr unsigned long long operator""_suffix(unsigned long long value) {
  return value;
}

#define RAW R"raw(a"b)raw"
#define NUMBER 0b1010'0101_suffix

int main(void) {
  RAW;
  return cxx_header_value() + (NUMBER == 165 ? 0 : 1);
}
