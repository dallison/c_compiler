// RUN: -std=c++26
// EXPECT: returned reference cannot be initialized with a temporary expression

int&& dangling() {
  return 42;
}
