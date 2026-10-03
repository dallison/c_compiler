// RUN: -std=c++20
// EXPECT_EXIT: 0

// A type with no common reference makes `common_reference_t<A, B>` ill-formed.
// A concept that names it as an argument (`same_as<common_reference_t<...>>`)
// or in a type-requirement is then false, not an error, so
// `assignable_from<counted_iterator&, default_sentinel_t>` is a constant.

#include <concepts>
#include <type_traits>

struct Sentinel {};

template <class I>
struct Counted {
  I current;
  Counted(I x) : current(x) {}

  template <class I2>
    requires std::is_assignable_v<I&, const I2&>
  Counted& operator=(const Counted<I2>& other) {
    current = other.current;
    return *this;
  }
};

template <class T, class U>
concept has_common_reference =
    requires { typename std::common_reference_t<T, U>; };

template <class I, class S>
int advance_to(I& it, S bound) {
  if constexpr (std::assignable_from<I&, S>) {
    it = static_cast<S&&>(bound);
    return 1;
  } else {
    return 2;
  }
}

int main() {
  int values[2] = {1, 2};
  Counted<int*> it(values);
  Counted<int*> end(values + 1);
  if (advance_to(it, Sentinel{}) != 2) {
    return 1;
  }
  if (advance_to(it, end) != 1 || it.current != values + 1) {
    return 2;
  }
  if (has_common_reference<const Counted<int*>&, const Sentinel&>) {
    return 3;
  }
  if (!has_common_reference<const int&, const long&>) {
    return 4;
  }
  if (std::common_reference_with<const Counted<int*>&, const Sentinel&>) {
    return 5;
  }
  return 0;
}
