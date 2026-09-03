// RUN: -std=c++20

#include <type_traits>

template <class First, class Second>
struct pair_type {};

template <class Second>
using int_pair = pair_type<int, Second>;

template <class Second>
constexpr bool deduce_alias_parameter(const int_pair<Second>&) {
  return std::is_same_v<Second, long>;
}

static_assert(deduce_alias_parameter(int_pair<long>{}));

int main() { return 0; }
