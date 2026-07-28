// RUN: -std=c++23
// EXPECT: 'if consteval' substatements must be compound statements

constexpr int select_context() {
  if consteval
    return 1;
  else
    return 2;
}
