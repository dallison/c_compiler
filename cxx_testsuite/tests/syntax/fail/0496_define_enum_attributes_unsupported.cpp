// RUN: -std=c++29
// EXPECT: attributes must be attribute reflections

#include <meta>

[[= ^^int ]] constexpr int annotation_source = 0;
constexpr auto attribute =
    std::meta::annotations_of(^^annotation_source)[0];
constexpr auto invalid = std::meta::enumerator_spec(
    {.name = "value", .attributes = {attribute}});
