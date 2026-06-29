// RUN: -std=c++20 -fno-exceptions
// EXPECT: cannot use 'throw' with exception handling disabled

int uses_throw(void) {
  throw 1;
}
