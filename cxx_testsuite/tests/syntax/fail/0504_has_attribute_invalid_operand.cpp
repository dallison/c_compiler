// RUN: -std=c++29
// EXPECT: static_assert expression is not an integer constant expression

#include <meta>

struct item {};

static_assert(std::meta::has_attribute(^^item, ^^int));
