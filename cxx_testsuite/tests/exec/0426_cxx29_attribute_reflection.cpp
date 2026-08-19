// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <meta>

struct empty {};
struct generated;
enum class generated_enum : int;

consteval {
  std::meta::define_aggregate(
      ^^generated,
      {std::meta::data_member_spec(
           ^^empty,
           {.name = "empty", .attributes = {^^[[no_unique_address]]}}),
       std::meta::data_member_spec(^^int, {.name = "value"})});
  std::meta::define_enum(
      ^^generated_enum,
      {std::meta::enumerator_spec(
           {.name = "old",
            .value = std::meta::reflect_constant(7),
            .attributes = {^^[[maybe_unused]]}}),
       std::meta::enumerator_spec({.name = "next"})});
}

static_assert(sizeof(generated) == sizeof(int));
static_assert(std::meta::has_attribute(^^generated_enum::old,
                                       ^^[[maybe_unused]]));
static_assert(std::meta::u8identifier_of(^^[[maybe_unused]]) ==
              u8"maybe_unused");
static_assert(std::meta::display_string_of(^^[[maybe_unused]]) ==
              "[[maybe_unused]]");

int main() {
  if (static_cast<int>(generated_enum::old) != 7 ||
      static_cast<int>(generated_enum::next) != 8) {
    return 1;
  }
  return 0;
}
