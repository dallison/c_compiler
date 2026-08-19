// RUN: -std=c++29
// EXPECT: must be a string-like or integral constant expression

void nonconstant_id(int x) {
  (void)^{ \id(x) };
}
