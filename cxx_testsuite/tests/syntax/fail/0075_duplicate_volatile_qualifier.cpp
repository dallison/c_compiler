// RUN: -std=c++20

volatile volatile int duplicate_volatile = 42;

int use_duplicate_volatile(void) {
  return duplicate_volatile;
}
