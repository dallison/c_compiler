#ifndef CXX_TESTSUITE_SYSTEM_PATH_UDL_HPP
#define CXX_TESTSUITE_SYSTEM_PATH_UDL_HPP

constexpr int operator""sysh(unsigned long long value) {
  return (int)value + 40;
}

#endif
