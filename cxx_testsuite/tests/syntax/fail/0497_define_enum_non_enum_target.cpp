// RUN: -std=c++29
// EXPECT: define_enum requires a reflected enumeration type

#include <meta>

constexpr auto value = std::meta::enumerator_spec({.name = "value"});

consteval {
  std::meta::define_enum(^^int, {value});
}
