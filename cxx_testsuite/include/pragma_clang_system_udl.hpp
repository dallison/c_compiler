#ifndef CXX_TESTSUITE_PRAGMA_CLANG_SYSTEM_UDL_HPP
#define CXX_TESTSUITE_PRAGMA_CLANG_SYSTEM_UDL_HPP

#pragma clang system_header

constexpr int operator""clangh(unsigned long long value) {
  return (int)value + 1;
}

#endif
