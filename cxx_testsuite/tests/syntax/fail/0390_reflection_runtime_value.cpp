// RUN: -std=c++26 -isystem libc/include
// EXPECT: A variable of consteval-only reflection type must be declared constexpr

#include <meta>

std::meta::info runtime_reflection = ^^int;
