// RUN: -std=c++29
// EXPECT: Attribute reflection requires exactly one attribute

constexpr auto attribute = ^^[[nodiscard, maybe_unused]];
