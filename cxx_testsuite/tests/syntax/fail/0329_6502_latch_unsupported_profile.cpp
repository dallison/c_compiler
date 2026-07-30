// RUN: -target 6502 -std=c++20
// EXPECT: <latch> is unavailable in this single-threaded target profile

#include <latch>
