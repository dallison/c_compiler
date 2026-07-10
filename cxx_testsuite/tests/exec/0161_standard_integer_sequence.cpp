// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <type_traits>
#include <utility>

using seq = std::integer_sequence<int, 2, 4, 6>;
using idx = std::index_sequence<0, 2, 4>;

static_assert(seq::size() == 3, "integer_sequence size");
static_assert(idx::size() == 3, "index_sequence size");
static_assert(std::is_same<typename seq::value_type, int>::value,
              "integer_sequence value_type");
static_assert(std::is_same<typename idx::value_type, size_t>::value,
              "index_sequence value_type");

template <class T, T A, T B, T C>
constexpr T sum(std::integer_sequence<T, A, B, C>) {
  return A + B + C;
}

int main(void) {
  if (sum<int, 2, 4, 6>(seq{}) != 12) {
    return 1;
  }
  if (sum<size_t, 0, 2, 4>(idx{}) != 6) {
    return 2;
  }
  return 0;
}
