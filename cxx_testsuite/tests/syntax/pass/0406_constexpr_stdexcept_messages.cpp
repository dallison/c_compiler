// RUN: -std=c++26 -fconstexpr-eval=audit

#include <stdexcept>
#include <string>

constexpr bool same_text(const char* left, const char* right) {
  for (int i = 0;; ++i) {
    if (left[i] != right[i]) {
      return false;
    }
    if (left[i] == '\0') {
      return true;
    }
  }
}

template <class Exception>
constexpr bool check_message(const char* message) {
  try {
    throw Exception(message);
  } catch (const std::exception& value) {
    return same_text(value.what(), message);
  }
}

constexpr bool check_copy_and_assignment() {
  try {
    throw std::logic_error(
        "a message long enough to require dynamically owned constexpr storage");
  } catch (const std::logic_error& value) {
    std::logic_error copy(value);
    std::logic_error assigned("temporary");
    assigned = value;
    return same_text(copy.what(), value.what()) &&
           same_text(assigned.what(), value.what());
  }
}

constexpr bool check_string_constructor() {
  std::string message("constexpr string message");
  try {
    throw std::runtime_error(message);
  } catch (const std::runtime_error& value) {
    return same_text(value.what(), message.c_str());
  }
}

static_assert(check_message<std::logic_error>("logic"));
static_assert(check_message<std::domain_error>("domain"));
static_assert(check_message<std::invalid_argument>("invalid"));
static_assert(check_message<std::length_error>("length"));
static_assert(check_message<std::out_of_range>("range"));
static_assert(check_message<std::runtime_error>("runtime"));
static_assert(check_message<std::range_error>("range"));
static_assert(check_message<std::overflow_error>("overflow"));
static_assert(check_message<std::underflow_error>("underflow"));
static_assert(check_copy_and_assignment());
static_assert(check_string_constructor());
