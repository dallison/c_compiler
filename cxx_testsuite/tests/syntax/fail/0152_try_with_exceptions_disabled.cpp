// RUN: -std=c++20 -fno-exceptions
// EXPECT: cannot use 'try' with exception handling disabled

int uses_try(int value) {
  try {
    value += 1;
  } catch (...) {
    value = 0;
  }
  return value;
}
