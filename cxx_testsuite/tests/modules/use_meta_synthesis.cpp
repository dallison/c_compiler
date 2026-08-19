import meta_synthesis;

#include <meta>

static_assert(imported_value == imported_value);
static_assert(std::meta::is_attribute(imported_attribute));
static_assert(std::meta::identifier_of(imported_attribute) == "nodiscard");
static_assert(std::meta::has_attribute(^^imported_attributed_type,
                                       imported_attribute));
static_assert(std::meta::identifier_of(imported_user_attribute) == "module");
static_assert(std::meta::has_attribute(^^imported_user_attributed_type,
                                       imported_user_attribute));
static_assert(std::meta::is_data_member_spec(imported_member));
static_assert(imported_designated_sum() == 52);
static_assert(std::meta::is_enumerator_spec(imported_negative));
static_assert(static_cast<int>(imported_enum::negative) == -3);
static_assert(static_cast<int>(imported_enum::zero) == -2);

constexpr auto imported_enumerators =
    std::meta::enumerators_of(^^imported_enum);
static_assert(imported_enumerators.size() == 2);
static_assert(std::meta::is_enumerator(imported_enumerators[0]));
static_assert(std::meta::has_attribute(imported_enumerators[0],
                                       ^^[[maybe_unused]]));
static_assert(static_cast<int>(std::meta::extract<imported_enum>(
                  std::meta::constant_of(imported_enumerators[1]))) == -2);

static_assert(imported_member ==
              std::meta::data_member_spec(
                  ^^int,
                  {.name = "member",
                   .attributes = {^^[[maybe_unused]]}}));

int main() { return 0; }
