// RUN: -std=c++29
// EXPECT: is not representable in the fixed underlying type

#include <meta>

enum class byte_value : unsigned char;

constexpr auto invalid = std::meta::enumerator_spec(
    {.name = "invalid", .value = std::meta::reflect_constant(-1)});

consteval {
  std::meta::define_enum(^^byte_value, {invalid});
}
