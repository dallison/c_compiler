// RUN: -std=c++29
// EXPECT: name must be a valid non-keyword C++ identifier

#include <meta>

constexpr auto invalid = std::meta::enumerator_spec({.name = "while"});
