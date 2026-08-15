// RUN: -std=c++26 -fconstexpr-eval=audit

#include <exception>
#include <new>
#include <typeinfo>

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
constexpr bool check_exception(const char* expected) {
  try {
    throw Exception{};
  } catch (const std::exception& value) {
    return same_text(value.what(), expected);
  }
}

static_assert(check_exception<std::exception>("std::exception"));
static_assert(check_exception<std::bad_exception>("std::bad_exception"));
static_assert(check_exception<std::bad_alloc>("std::bad_alloc"));
static_assert(
    check_exception<std::bad_array_new_length>("std::bad_array_new_length"));
static_assert(check_exception<std::bad_cast>("std::bad_cast"));
static_assert(check_exception<std::bad_typeid>("std::bad_typeid"));
