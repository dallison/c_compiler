// RUN: -std=c++29
// EXPECT: the operand type of a defaulted postfix operator must be complete

struct counter;

counter operator++(counter&, int) = default;
