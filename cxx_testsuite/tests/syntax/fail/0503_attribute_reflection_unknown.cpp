// RUN: -std=c++29
// EXPECT: Attribute 'unknown_attribute' is not reflectable

constexpr auto attribute = ^^[[unknown_attribute]];
