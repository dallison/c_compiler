// RUN: -std=c++29
// EXPECT: exceeds the supported integer constant range

#include <meta>

enum class overflow : long long;

constexpr auto maximum = std::meta::enumerator_spec(
    {.name = "maximum",
     .value = std::meta::reflect_constant(9223372036854775807LL)});
constexpr auto next = std::meta::enumerator_spec({.name = "next"});

consteval {
  std::meta::define_enum(^^overflow, {maximum, next});
}
