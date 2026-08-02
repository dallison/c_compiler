// RUN: -std=c++20
// EXPECT: lambda specifiers without a parameter list require C++23
// EXPECT: static lambda requires C++23

auto function = [] static { return 1; };
