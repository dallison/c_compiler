// RUN: -std=c++29
// EXPECT: attributes require P3385 attribute reflection support

#include <meta>

[[= ^^int ]] constexpr int annotation_source = 0;
constexpr auto attribute =
    std::meta::annotations_of(^^annotation_source)[0];
constexpr auto invalid = std::meta::enumerator_spec(
    {.name = "value", .attributes = {attribute}});
