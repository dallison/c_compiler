// RUN: -std=c++23
// EXPECT_EXIT: 0

// Assigning a value to an expected holding an error goes through a member
// template that writes through `operator*`.  Its clone must keep the `&`
// overload, not the first-declared `const&` one.
#include <expected>

int main() {
  std::expected<int, int> error(std::unexpect, 12);
  error = 19;
  if (!error.has_value() || *error != 19) {
    return 1;
  }
  std::expected<int, int> value(3);
  value = std::unexpected(8);
  if (value.has_value() || value.error() != 8) {
    return 2;
  }
  value = 4;
  *value += 1;
  return *value == 5 ? 0 : 3;
}
