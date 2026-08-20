// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <expected>
#include <version>

#if __cpp_lib_expected != 202606L
#error "__cpp_lib_expected has the wrong value"
#endif

constexpr bool test_value_state() {
  std::expected<int, int> value(42);
  std::expected<void, int> void_value;

  return value.has_value() && !value.has_error() &&
         void_value.has_value() && !void_value.has_error();
}

static_assert(test_value_state());
static_assert(noexcept(std::expected<int, int>{}.has_error()));

int main() {
  std::expected<int, int> error(std::unexpect, 7);
  std::expected<void, int> void_error(std::unexpect, 9);
  if (error.has_value() || !error.has_error()) {
    return 1;
  }
  if (void_error.has_value() || !void_error.has_error()) {
    return 2;
  }
  return test_value_state() ? 0 : 3;
}
