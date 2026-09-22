// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <typename T, typename V>
using Elem =
    typename std::conditional<std::is_reference<T>::value,
                              std::is_convertible<V, T>,
                              std::is_constructible<T, V&&>>::type;

template <class T, class V>
using Flag = typename std::conditional<std::is_same<T, V>::value, std::true_type,
                                       std::false_type>::type;

template <class... Ts>
struct PackMove
    : std::integral_constant<bool,
                             std::conjunction<Elem<Ts, Ts&&>...>::value> {};

template <class T>
struct OneMove
    : std::integral_constant<bool, std::conjunction<Elem<T, T&&>>::value> {};

template <class T, class V>
struct FlagMove
    : std::integral_constant<bool, std::conjunction<Flag<T, V>>::value> {};

template <class... Ts>
struct TraitPack
    : std::integral_constant<
          bool, std::conjunction<std::is_constructible<Ts, Ts&&>...>::value> {};

int main() {
  if (!PackMove<int, char>::value) {
    return 1;
  }
  if (!OneMove<int>::value) {
    return 2;
  }
  if (!TraitPack<int, char>::value) {
    return 3;
  }
  if (!FlagMove<int, int>::value) {
    return 4;
  }
  if (FlagMove<int, char>::value) {
    return 5;
  }
  return 0;
}
