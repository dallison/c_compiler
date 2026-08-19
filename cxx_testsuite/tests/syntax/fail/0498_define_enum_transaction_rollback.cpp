// RUN: -std=c++29
// EXPECT: static assertion failed

#include <meta>

enum class rollback_target;

constexpr auto discarded =
    std::meta::enumerator_spec({.name = "discarded"});
constexpr auto retained =
    std::meta::enumerator_spec({.name = "retained"});

consteval {
  consteval {
    std::meta::define_enum(^^rollback_target, {discarded});
  }
  static_assert(false);
}

consteval {
  std::meta::define_enum(^^rollback_target, {retained});
}

static_assert(static_cast<int>(rollback_target::retained) == 0);
