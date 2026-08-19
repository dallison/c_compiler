// RUN: -std=c++29

#include <meta>

using namespace std::meta;

struct [[acme::entity("record", 42)]] record {
  [[acme::field(read, write)]] int value;
};

constexpr auto entity = ^^[[acme::entity("record", 42)]];
constexpr auto other_entity = ^^[[other::entity("record", 42)]];

static_assert(is_attribute(entity));
static_assert(identifier_of(entity) == "entity");
static_assert(display_string_of(entity).size() > 0);
static_assert(^^[[acme::entity("record", 42)]] ==
              ^^[[acme::entity("record", 42)]]);
static_assert(^^[[acme::entity("record", 42)]] !=
              ^^[[acme::entity("record", 7)]]);
static_assert(^^[[acme::entity("record", 42)]] !=
              ^^[[other::entity("record", 42)]]);

static_assert(has_attribute(^^record, entity));
static_assert(!has_attribute(^^record, other_entity));
static_assert(has_attribute(^^record, other_entity,
                            attribute_comparison::ignore_namespace));
static_assert(has_attribute(^^record, ^^[[acme::entity]],
                            attribute_comparison::ignore_argument));

constexpr auto record_attributes = attributes_of(^^record);
static_assert(record_attributes.size() == 1);
static_assert(record_attributes[0] == entity);
static_assert(has_attribute(^^record::value,
                            ^^[[acme::field(read, write)]]));

struct generated;
enum class generated_enum : int;

consteval {
  define_aggregate(
      ^^generated,
      {data_member_spec(
          ^^int,
          {.name = "value", .attributes = {^^[[acme::generated("member")]]}})});
  define_enum(
      ^^generated_enum,
      {enumerator_spec(
          {.name = "value",
           .attributes = {^^[[acme::generated("enumerator")]]}})});
}

constexpr auto generated_members = nonstatic_data_members_of(^^generated);
static_assert(generated_members.size() == 1);
static_assert(has_attribute(generated_members[0],
                            ^^[[acme::generated("member")]]));
static_assert(has_attribute(^^generated_enum::value,
                            ^^[[acme::generated("enumerator")]]));

int main() { return 0; }
