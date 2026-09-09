// RUN: -std=c++20

#include <utility>

template <size_t... I>
constexpr size_t count(std::index_sequence<I...>) {
  return sizeof...(I);
}

template <class... Types>
constexpr size_t count_types() {
  return count(std::make_index_sequence<sizeof...(Types)>{});
}

int main() {
  if (count(std::make_index_sequence<3>{}) != 3) {
    return 1;
  }
  if (count_types<int, char, long, unsigned>() != 4) {
    return 2;
  }
  using Seq = std::make_index_sequence<2>;
  if (count(Seq{}) != 2) {
    return 3;
  }
  return 0;
}
