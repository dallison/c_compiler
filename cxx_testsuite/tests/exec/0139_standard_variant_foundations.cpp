// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>
#include <type_traits>
#include <utility>

struct Tag {
  int value;
  explicit Tag(int v) : value(v) {}
};

int main(void) {
  static_assert(std::variant_size<std::variant<int, char, long> >::value == 3,
                "variant_size");
  static_assert(std::variant_size_v<std::variant<int, char, long> > == 3,
                "variant_size_v");
  static_assert(
      std::variant_size<const std::variant<int, char, long> >::value == 3,
      "variant_size const");
  static_assert(
      std::variant_size<volatile std::variant<int, char, long> >::value == 3,
      "variant_size volatile");
  static_assert(std::variant_size<const volatile std::variant<int, char, long> >
                    ::value == 3,
                "variant_size const volatile");
  static_assert(std::is_same<
                    std::variant_alternative_t<1,
                                               std::variant<int, char, long> >,
                    char>::value,
                "variant_alternative_t");
  static_assert(
      std::is_same<
          std::variant_alternative_t<1,
                                     const std::variant<int, char, long> >,
          const char>::value,
      "variant_alternative_t const");
  static_assert(
      std::is_same<
          std::variant_alternative_t<1,
                                     volatile std::variant<int, char, long> >,
          volatile char>::value,
      "variant_alternative_t volatile");
  static_assert(std::is_same<
                    std::variant_alternative_t<
                        1, const volatile std::variant<int, char, long> >,
                    const volatile char>::value,
                "variant_alternative_t const volatile");

  std::variant<std::monostate, int> empty;
  if (empty.index() != 0 || !std::holds_alternative<std::monostate>(empty)) {
    return 1;
  }
  if (!(std::monostate{} == std::monostate{}) ||
      std::monostate{} < std::monostate{}) {
    return 2;
  }
  if ((std::monostate{} <=> std::monostate{}) !=
      std::strong_ordering::equal) {
    return 3;
  }

  std::variant<int, Tag> by_index(std::in_place_index<1>, 42);
  if (by_index.index() != 1 || std::get<1>(by_index).value != 42) {
    return 4;
  }

  std::variant<int, Tag> by_type(std::in_place_type<Tag>, 7);
  if (by_type.index() != 1 || std::get<Tag>(by_type).value != 7) {
    return 5;
  }

  bool threw = false;
  try {
    std::get<0>(by_type);
  } catch (const std::bad_variant_access&) {
    threw = true;
  }
  if (!threw) {
    return 6;
  }

  if (std::variant_npos != static_cast<size_t>(-1)) {
    return 7;
  }

  return 0;
}
