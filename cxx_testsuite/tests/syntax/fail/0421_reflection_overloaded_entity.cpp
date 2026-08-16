// RUN: -std=c++26
// EXPECT: An overloaded entity cannot be reflected

void f(int) {}
void f(double) {}

constexpr auto r = ^^f;
