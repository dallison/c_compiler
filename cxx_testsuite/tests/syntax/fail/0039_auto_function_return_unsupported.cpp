// RUN: -std=c++20
// EXPECT: Illegal conversion

auto value(void);
int before_definition = value();
auto value(void) { return 1; }
