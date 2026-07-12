#ifndef CXX_TESTSUITE_PRAGMA_SYSTEM_UDL_HPP
#define CXX_TESTSUITE_PRAGMA_SYSTEM_UDL_HPP

#pragma GCC system_header

int operator""s(unsigned long long value) {
  return (int)value + 1000;
}

int operator""us(unsigned long long value) {
  return (int)value + 2000;
}

int operator""cstr(const char* text, unsigned long size) {
  return (int)(text[0] + text[1] + size);
}

#endif
