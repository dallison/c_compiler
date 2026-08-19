// RUN: -std=c++29
// EXPECT: value must be an integral reflection constant

#include <meta>

constexpr auto invalid = std::meta::enumerator_spec(
    {.name = "value", .value = std::meta::reflect_constant(1.5)});
