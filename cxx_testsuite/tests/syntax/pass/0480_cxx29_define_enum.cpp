// RUN: -std=c++29

#include <meta>
#include <version>

#ifndef __davecc_p4033_define_enum
#error "P4033 define_enum vendor probe is missing"
#endif

using namespace std::meta;

enum class synthesized : signed char;

constexpr auto minus_two =
    enumerator_spec({.name = "minus_two", .value = reflect_constant(-2)});
constexpr auto next = enumerator_spec({.name = "next"});
constexpr auto ten =
    enumerator_spec({.name = "ten", .value = reflect_constant(10)});
constexpr auto eleven = enumerator_spec({.name = "eleven"});

static_assert(is_enumerator_spec(minus_two));
static_assert(!is_enumerator_spec(^^int));

consteval {
  define_enum(^^synthesized, {minus_two, next, ten, eleven});
}

static_assert(static_cast<int>(synthesized::minus_two) == -2);
static_assert(static_cast<int>(synthesized::next) == -1);
static_assert(static_cast<int>(synthesized::ten) == 10);
static_assert(static_cast<int>(synthesized::eleven) == 11);

constexpr auto reflected = enumerators_of(^^synthesized);
static_assert(reflected.size() == 4);
static_assert(is_enumerator(reflected[0]));
static_assert(extract<int>(constant_of(reflected[0])) == -2);

[[= ^^int ]] constexpr int annotation_source = 0;
constexpr auto annotation = annotations_of(^^annotation_source)[0];
constexpr auto ranged_spec =
    enumerator_spec({.name = "from_range", .annotations = {annotation}});
constexpr info ranged_specs[] = {ranged_spec};

enum class ranged;

consteval {
  define_enum(^^ranged, ranged_specs);
}

static_assert(static_cast<int>(ranged::from_range) == 0);
static_assert(annotations_of(^^ranged::from_range).size() == 1);

int main() { return 0; }
