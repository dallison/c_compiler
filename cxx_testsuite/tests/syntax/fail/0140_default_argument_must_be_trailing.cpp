// RUN: -std=c++20
// EXPECT: parameter after default argument must have a default argument
int bad_default_order(int value = 1, int missing);
