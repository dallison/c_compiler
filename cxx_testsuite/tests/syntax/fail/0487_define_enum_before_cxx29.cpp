// RUN: -std=c++26
// EXPECT: No such symbol "std::meta::enumerator_spec"

#include <meta>

constexpr auto invalid =
    std::meta::enumerator_spec({.name = "not_available"});
