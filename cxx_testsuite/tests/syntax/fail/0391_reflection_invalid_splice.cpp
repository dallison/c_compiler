// RUN: -std=c++26
// EXPECT: Splice operand is not a constant reflection

constexpr int not_reflection = 0;

int value = [:not_reflection:];
