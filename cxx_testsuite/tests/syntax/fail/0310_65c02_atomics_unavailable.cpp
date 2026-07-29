// RUN: -target 65c02 -std=c++20
// EXPECT: <atomic> is unavailable in the single-threaded 65(C)02 profile

#include <atomic>

int main() {}
