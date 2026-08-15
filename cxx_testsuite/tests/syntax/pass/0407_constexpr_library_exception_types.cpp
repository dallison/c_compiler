// RUN: -std=c++26 -fconstexpr-eval=audit

#include <expected>
#include <format>
#include <optional>
#include <variant>

constexpr bool check_optional() {
  try {
    std::optional<int> value;
    (void)value.value();
  } catch (const std::bad_optional_access& value) {
    return value.what()[0] == 'b';
  }
  return false;
}

constexpr bool check_variant() {
  try {
    std::variant<int, long> value(1);
    (void)std::get<1>(value);
  } catch (const std::bad_variant_access& value) {
    return value.what()[0] == 'b';
  }
  return false;
}

constexpr bool check_expected() {
  try {
    std::expected<int, int> value(std::unexpected<int>(23));
    (void)value.value();
  } catch (const std::bad_expected_access<int>& value) {
    return value.error() == 23 && value.what()[0] == 'b';
  }
  return false;
}

constexpr bool check_format_error() {
  try {
    std::format_parse_context context("", 0);
    (void)context.next_arg_id();
  } catch (const std::format_error& value) {
    return value.what()[0] == 'f';
  }
  return false;
}

static_assert(check_optional());
static_assert(check_variant());
static_assert(check_expected());
static_assert(check_format_error());
