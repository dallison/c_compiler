// RUN: -std=c++20
// EXPECT: lambda specifiers without a parameter list require C++23

auto function = [] mutable { return 1; };
