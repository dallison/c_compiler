// RUN: -std=c++29
// EXPECT: define_enum requires an active consteval block

#include <meta>

enum class incomplete;

constexpr auto value = std::meta::enumerator_spec({.name = "value"});
constexpr auto completed = std::meta::define_enum(^^incomplete, {value});
