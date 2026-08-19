module;
#include <meta>

export module meta_synthesis;

export using info = std::meta::info;
export constexpr auto imported_value = std::meta::reflect_constant(17);
export constexpr auto imported_attribute = ^^[[nodiscard("module")]];
export constexpr auto imported_user_attribute = ^^[[acme::module("persist")]];
export constexpr auto imported_member = std::meta::data_member_spec(
    ^^int, {.name = "member", .attributes = {^^[[maybe_unused]]}});

export struct [[nodiscard("module")]] imported_attributed_type {};
export struct [[acme::module("persist")]] imported_user_attributed_type {};

export enum class imported_enum : int;

export constexpr auto imported_negative = std::meta::enumerator_spec(
    {.name = "negative",
     .value = std::meta::reflect_constant(-3),
     .attributes = {^^[[maybe_unused]]}});
constexpr auto imported_zero = std::meta::enumerator_spec({.name = "zero"});

consteval {
  std::meta::define_enum(^^imported_enum,
                         {imported_negative, imported_zero});
}
