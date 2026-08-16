// RUN: -target 6502 -std=c++23
// EXPECT: binary64 extended floating-point type is not supported on this target

auto value = 1.0f64;
