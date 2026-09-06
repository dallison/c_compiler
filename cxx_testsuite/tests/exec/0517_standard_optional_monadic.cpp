// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <optional>
#include <type_traits>
#include <version>

#if __cpp_lib_optional != 202506L
#error "incorrect __cpp_lib_optional"
#endif

static_assert(std::is_same<std::optional<int&>::value_type, int>::value,
              "optional<T&>::value_type is T");

std::optional<int> times_two(int value) { return std::optional<int>(value * 2); }

int plus_one(int value) { return value + 1; }

std::optional<int> forty_two() { return std::optional<int>(42); }

int fallback_number = 11;

std::optional<int&> fallback_ref() { return std::optional<int&>(fallback_number); }

int main() {
  std::optional<int> value(3);
  std::optional<int> doubled = value.and_then(times_two);
  if (!doubled || *doubled != 6) return 1;

  std::optional<int> empty;
  if (empty.and_then(times_two).has_value()) return 2;

  std::optional<int> mapped = value.transform(plus_one);
  if (!mapped || *mapped != 4) return 3;
  if (empty.transform(plus_one).has_value()) return 4;

  std::optional<int> filled = empty.or_else(forty_two);
  if (!filled || *filled != 42) return 5;
  std::optional<int> kept = value.or_else(forty_two);
  if (!kept || *kept != 3) return 6;

  int number = 8;
  std::optional<int&> alias(number);
  if (!alias || *alias != 8) return 7;
  *alias = 9;
  if (number != 9 || alias.value() != 9) return 8;

  std::optional<int> from_ref = alias.transform(plus_one);
  if (!from_ref || *from_ref != 10) return 9;

  alias.reset();
  if (alias.has_value() || number != 9) return 10;
  std::optional<int&> rebound = alias.or_else(fallback_ref);
  if (!rebound || *rebound != 11) return 11;

  return 0;
}
