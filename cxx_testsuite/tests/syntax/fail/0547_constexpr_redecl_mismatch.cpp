// RUN: -std=c++20
// EXPECT: 'constexpr' and 'consteval' specifiers must match previous declaration

constexpr int f();
consteval int f() {
  return 1;
}
