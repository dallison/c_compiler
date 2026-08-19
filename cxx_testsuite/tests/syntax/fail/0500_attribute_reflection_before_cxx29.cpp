// RUN: -std=c++26
// EXPECT: Attribute reflection requires C++29

constexpr auto attribute = ^^[[nodiscard]];
