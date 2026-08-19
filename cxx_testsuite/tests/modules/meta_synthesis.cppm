module;
#include <meta>

export module meta_synthesis;

export using info = std::meta::info;
export constexpr auto imported_value = std::meta::reflect_constant(17);

export enum class imported_enum : int;

export constexpr auto imported_negative = std::meta::enumerator_spec(
    {.name = "negative", .value = std::meta::reflect_constant(-3)});
constexpr auto imported_zero = std::meta::enumerator_spec({.name = "zero"});

consteval {
  std::meta::define_enum(^^imported_enum,
                         {imported_negative, imported_zero});
}
