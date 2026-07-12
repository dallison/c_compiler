// RUN: -std=c++20
// EXPECT: Literal operator suffix "reserved" must begin with '_' outside a system header

int operator""reserved(unsigned long long value) {
  return (int)value;
}
