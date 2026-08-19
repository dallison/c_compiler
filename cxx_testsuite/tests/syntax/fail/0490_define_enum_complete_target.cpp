// RUN: -std=c++29
// EXPECT: define_enum target enumeration is already complete

#include <meta>

enum class complete { existing };

constexpr auto value = std::meta::enumerator_spec({.name = "value"});

consteval {
  std::meta::define_enum(^^complete, {value});
}
