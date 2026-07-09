// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

constexpr int default_variant_index(void) {
  std::variant<int, long> value;
  return static_cast<int>(value.index());
}

constexpr int default_variant_get_if_value(void) {
  std::variant<int, long> value;
  return *std::get_if<0>(&value);
}

constexpr int default_variant_get_value(void) {
  std::variant<int, long> value;
  return std::get<0>(value);
}

constexpr int in_place_index_variant_value(void) {
  std::variant<int, long> value(std::in_place_index<1>, 42L);
  return static_cast<int>(std::get<1>(value));
}

constexpr int in_place_index_variant_index(void) {
  std::variant<int, long> value(std::in_place_index<1>, 42L);
  return static_cast<int>(value.index());
}

constexpr int in_place_type_variant_value(void) {
  std::variant<int, long> value(std::in_place_type<long>, 43L);
  return static_cast<int>(std::get<long>(value));
}

constexpr int in_place_type_variant_index_value(void) {
  std::variant<int, long> value(std::in_place_type<long>, 44L);
  return static_cast<int>(std::get<1>(value));
}

constexpr int in_place_type_variant_index(void) {
  std::variant<int, long> value(std::in_place_type<long>, 43L);
  return static_cast<int>(value.index());
}

constexpr bool default_variant_not_valueless(void) {
  std::variant<int, long> value;
  return !value.valueless_by_exception();
}

constexpr bool default_variant_holds_int(void) {
  std::variant<int, long> value;
  return std::holds_alternative<int>(value) &&
         !std::holds_alternative<long>(value);
}

constexpr bool in_place_variant_holds_long(void) {
  std::variant<int, long> value(std::in_place_type<long>, 49L);
  return std::holds_alternative<long>(value) &&
         !std::holds_alternative<int>(value);
}

constexpr int copy_construct_variant_value(void) {
  std::variant<int, long> source(std::in_place_index<1>, 45L);
  std::variant<int, long> copy(source);
  return static_cast<int>(std::get<1>(copy));
}

constexpr int copy_assign_variant_value(void) {
  std::variant<int, long> source(std::in_place_index<1>, 46L);
  std::variant<int, long> copy;
  copy = source;
  return static_cast<int>(copy.index() + std::get<1>(copy));
}

constexpr int copy_assign_variant_index(void) {
  std::variant<int, long> source(std::in_place_index<1>, 46L);
  std::variant<int, long> copy;
  copy = source;
  return static_cast<int>(copy.index());
}

constexpr int copy_assign_variant_get(void) {
  std::variant<int, long> source(std::in_place_index<1>, 46L);
  std::variant<int, long> copy;
  copy = source;
  return static_cast<int>(std::get<1>(copy));
}

constexpr int value_assign_variant_value(void) {
  std::variant<int, long> value;
  value = 47L;
  return static_cast<int>(value.index() + std::get<1>(value));
}

constexpr int value_assign_variant_index(void) {
  std::variant<int, long> value;
  value = 47L;
  return static_cast<int>(value.index());
}

constexpr int value_assign_variant_get(void) {
  std::variant<int, long> value;
  value = 47L;
  return static_cast<int>(std::get<1>(value));
}

constexpr int emplace_variant_value(void) {
  std::variant<int, long> value;
  long& result = value.emplace<1>(48L);
  return static_cast<int>(value.index() + result);
}

constexpr bool variant_equality_comparison(void) {
  std::variant<int, long> one(1);
  std::variant<int, long> another_one(1);
  std::variant<int, long> two(2);
  return one == another_one && one != two;
}

constexpr bool variant_ordering_comparison(void) {
  std::variant<int, long> one(1);
  std::variant<int, long> two(2);
  std::variant<int, long> long_zero(std::in_place_index<1>, 0L);
  return one < two && two > one && one <= two && two >= one &&
         two < long_zero;
}

constexpr bool variant_spaceship_comparison(void) {
  std::variant<int, long> one(1);
  std::variant<int, long> another_one(1);
  std::variant<int, long> two(2);
  std::variant<int, long> long_zero(std::in_place_index<1>, 0L);
  return (one <=> another_one) == std::strong_ordering::equal &&
         (one <=> two) == std::strong_ordering::less &&
         (long_zero <=> two) == std::strong_ordering::greater;
}

static_assert(default_variant_index() == 0, "constexpr variant index");
static_assert(default_variant_get_if_value() == 0, "constexpr variant get_if");
static_assert(default_variant_get_value() == 0, "constexpr variant get");
static_assert(in_place_index_variant_index() == 1,
              "constexpr variant in-place index index");
static_assert(in_place_index_variant_value() == 42,
              "constexpr variant in-place index");
static_assert(in_place_type_variant_index() == 1,
              "constexpr variant in-place type index");
static_assert(in_place_type_variant_index_value() == 44,
              "constexpr variant in-place type index get");
static_assert(in_place_type_variant_value() == 43,
              "constexpr variant in-place type");
static_assert(default_variant_not_valueless(),
              "constexpr variant not valueless");
static_assert(default_variant_holds_int(), "constexpr variant holds int");
static_assert(in_place_variant_holds_long(), "constexpr variant holds long");
static_assert(copy_construct_variant_value() == 45,
              "constexpr variant copy construct");
static_assert(copy_assign_variant_index() == 1,
              "constexpr variant copy assign index");
static_assert(copy_assign_variant_get() == 46,
              "constexpr variant copy assign get");
static_assert(copy_assign_variant_value() == 47,
              "constexpr variant copy assign combined");
static_assert(value_assign_variant_index() == 1,
              "constexpr variant value assign index");
static_assert(value_assign_variant_get() == 47,
              "constexpr variant value assign get");
static_assert(value_assign_variant_value() == 48,
              "constexpr variant value assign combined");
static_assert(emplace_variant_value() == 49, "constexpr variant emplace");
static_assert(variant_equality_comparison(), "constexpr variant equality");
static_assert(variant_ordering_comparison(), "constexpr variant ordering");
static_assert(variant_spaceship_comparison(), "constexpr variant spaceship");

int main(void) {
  return 0;
}
