// RUN: -std=c++20
// EXPECT: Literal operator suffix "leaked" must begin with '_' outside a system header

#include "cxx_testsuite/include/pragma_system_udl.hpp"

int operator""leaked(unsigned long long value) {
  return (int)value;
}
