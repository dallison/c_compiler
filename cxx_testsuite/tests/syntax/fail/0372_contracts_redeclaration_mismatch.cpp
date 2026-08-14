// RUN: -std=c++26
// EXPECT: have non-corresponding contract assertions

int mismatched(const int value)
    pre (value > 0);

int mismatched(const int value)
    pre (value >= 0) {
  return value;
}
