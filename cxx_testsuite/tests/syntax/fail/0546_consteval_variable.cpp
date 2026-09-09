// RUN: -std=c++20
// EXPECT: 'consteval' can only be applied to functions

consteval int value = 1;
