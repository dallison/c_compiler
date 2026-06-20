// RUN: -std=c++20
// EXPECT: throw without operand is only valid in a catch handler

void value(void) {
  throw;
}
