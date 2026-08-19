// RUN: -std=c++29
// EXPECT: static_assert expression is not an integer constant expression

#include <meta>

struct [[nodiscard]] item {};

static_assert(std::meta::has_attribute(
    ^^item, ^^[[nodiscard]],
    static_cast<std::meta::attribute_comparison>(4)));
