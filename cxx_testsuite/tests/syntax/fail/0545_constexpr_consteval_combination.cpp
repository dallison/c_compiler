// RUN: -std=c++20
// EXPECT: 'constexpr' and 'consteval' cannot both be specified

constexpr consteval int both() {
  return 1;
}
