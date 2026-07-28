// RUN: -std=c++20
// EXPECT: 'constexpr' and 'constinit' cannot be combined

constexpr constinit int value = 1;
