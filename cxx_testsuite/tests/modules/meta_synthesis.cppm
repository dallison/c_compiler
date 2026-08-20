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

export struct imported_designated_base {
  int base_value;
};

export struct imported_designated_derived : imported_designated_base {
  int derived_value;
};

export constexpr imported_designated_derived make_imported_designated() {
  return {.base_value = 23, .derived_value = 29};
}

export constexpr int imported_designated_sum() {
  imported_designated_derived value{
      .base_value = 23, .derived_value = 29};
  return value.base_value + value.derived_value;
}

export struct imported_nothrow_callable {
  int operator()() noexcept;
};

export struct imported_throwing_callable {
  int operator()();
};

export template <typename F, bool RequireNoexcept>
concept imported_callable_as_requested = requires(F function) {
  { function() } noexcept(RequireNoexcept);
};

export template <typename T>
struct imported_first_template {
  static constexpr int id = 1;
};

export template <typename T>
struct imported_second_template {
  static constexpr int id = 2;
};

export template <template <typename> typename... Templates>
struct imported_template_selector {
  using selected = Templates...[1]<int>;
};

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
