// RUN: -std=c++20
// EXPECT: 'constexpr' and 'consteval' specifiers must match previous declaration

consteval int g();
constexpr int g() {
  return 1;
}
