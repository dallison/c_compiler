// RUN: -target 65c02 -std=c++20
// EXPECT: <thread> is unavailable in this single-threaded target profile

#include <thread>
