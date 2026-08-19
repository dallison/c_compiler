// RUN: -std=c++29
// EXPECT: define_enum requires enumerator descriptions

#include <meta>

enum class malformed;

consteval {
  std::meta::define_enum(^^malformed, {^^int});
}
