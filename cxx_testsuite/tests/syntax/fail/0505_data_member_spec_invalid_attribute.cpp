// RUN: -std=c++29
// EXPECT: data_member_spec: attributes must be attribute reflections

#include <meta>

constexpr auto invalid = std::meta::data_member_spec(
    ^^int, {.name = "value", .attributes = {^^int}});
