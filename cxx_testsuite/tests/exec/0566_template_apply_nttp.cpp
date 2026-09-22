// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class...>
struct disjunction : std::true_type {};

struct HashSelect {
  template <typename T>
  using Apply = disjunction<std::true_type>;
};

template <typename T>
struct is_hashable
    : std::integral_constant<bool, HashSelect::template Apply<T>::value> {};

int main() { return is_hashable<int>::value ? 0 : 1; }
