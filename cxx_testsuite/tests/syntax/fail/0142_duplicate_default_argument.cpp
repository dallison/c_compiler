// RUN: -std=c++20
// EXPECT: default argument already specified
int duplicate_default(int value = 1);
int duplicate_default(int value = 2);
