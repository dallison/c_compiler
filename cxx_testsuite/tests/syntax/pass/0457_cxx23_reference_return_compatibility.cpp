// RUN: -std=c++23

// This is well-formed before C++26, although using its result would dangle.
int&& legacy_dangling_return() {
  return 42;
}
