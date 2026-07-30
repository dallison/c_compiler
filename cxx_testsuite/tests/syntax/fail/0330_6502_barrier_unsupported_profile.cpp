// RUN: -target 6502 -std=c++20
// EXPECT: <barrier> is unavailable in this single-threaded target profile

#include <barrier>
