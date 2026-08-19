// RUN: -std=c++29

#include <meta>
#include <version>

using namespace std::meta;

#ifndef __davecc_p3385_attribute_reflection
#error "attribute reflection feature probe is missing"
#endif

struct [[nodiscard("keep")]] reflected_type {
  [[deprecated("old field")]] int old_field;
  [[maybe_unused]] int quiet_field;
};

[[gnu::deprecated("gnu old")]] int old_function();

constexpr auto nodiscard_keep = ^^[[nodiscard("keep")]];
constexpr auto nodiscard_other = ^^[[nodiscard("other")]];
constexpr auto deprecated_attribute = ^^[[deprecated]];

static_assert(is_attribute(nodiscard_keep));
static_assert(!is_attribute(^^reflected_type));
static_assert(has_identifier(nodiscard_keep));
static_assert(identifier_of(nodiscard_keep) == "nodiscard");
static_assert(^^[[nodiscard("keep")]] == ^^[[nodiscard("keep")]]);
static_assert(^^[[nodiscard("keep")]] != ^^[[nodiscard("other")]]);
static_assert(^^[[nodiscard("keep")]] != ^^[[deprecated]]);

constexpr auto type_attributes = attributes_of(^^reflected_type);
static_assert(type_attributes.size() == 1);
static_assert(type_attributes[0] == nodiscard_keep);
static_assert(has_attribute(^^reflected_type, nodiscard_keep));
static_assert(!has_attribute(^^reflected_type, nodiscard_other));
static_assert(has_attribute(^^reflected_type, ^^[[nodiscard]],
                            attribute_comparison::ignore_argument));

static_assert(has_attribute(^^old_function, ^^[[deprecated]],
                            attribute_comparison::ignore_namespace |
                                attribute_comparison::ignore_argument));
static_assert(!has_attribute(^^old_function, ^^[[deprecated]]));

constexpr auto members = nonstatic_data_members_of(^^reflected_type);
static_assert(members.size() == 2);
static_assert(has_attribute(members[0], ^^[[deprecated("old field")]]));
static_assert(has_attribute(members[1], ^^[[maybe_unused]]));

struct empty {};
struct generated;
struct generated_trailing;
enum class generated_enum : int;

consteval {
  define_aggregate(
      ^^generated,
      {data_member_spec(^^empty,
                        {.name = "empty",
                         .attributes = {^^[[no_unique_address]]}}),
       data_member_spec(^^char, {.name = "value"})});
  define_aggregate(
      ^^generated_trailing,
      {data_member_spec(^^char, {.name = "value"}),
       data_member_spec(^^empty,
                        {.name = "empty",
                         .attributes = {^^[[no_unique_address]]}})});
  define_enum(
      ^^generated_enum,
      {enumerator_spec(
           {.name = "old",
            .value = reflect_constant(0),
            .attributes = {^^[[maybe_unused]]}}),
       enumerator_spec({.name = "current"})});
}

static_assert(sizeof(generated) == 1);
static_assert(sizeof(generated_trailing) == 1);
static_assert(has_attribute(^^generated_enum::old, ^^[[maybe_unused]]));
static_assert(attributes_of(^^generated_enum::current).size() == 0);

int main() { return 0; }
