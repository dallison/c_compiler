// RUN: -std=c++29
// EXPECT: duplicate enumerator name 'same'

#include <meta>

enum class duplicate;

constexpr auto first = std::meta::enumerator_spec({.name = "same"});
constexpr auto second = std::meta::enumerator_spec({.name = "same"});

consteval {
  std::meta::define_enum(^^duplicate, {first, second});
}
