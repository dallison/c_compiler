// RUN: -std=c++29
// EXPECT: define_enum requires a scoped enumeration type

#include <meta>

enum unscoped;

constexpr auto value = std::meta::enumerator_spec({.name = "value"});

consteval {
  std::meta::define_enum(^^unscoped, {value});
}
