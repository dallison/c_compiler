// RUN: -std=c++20
// EXPECT: aligned attribute argument must be an integer

[[gnu::aligned("wide")]] int invalid_alignment;
