// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <memory>
#include <type_traits>
#include <utility>

template <class T>
struct Policy {
  static T& value(T*);
};

template <class T>
struct Base {
  using reference = T&;
};

template <class P, class... Params>
struct Container : Base<int> {
  template <class Q>
  using MappedReference = decltype(
      Q::value(std::addressof(
          std::declval<typename Container::reference>())));

  template <class Key = int, class Q = P>
  MappedReference<Q> get(Key&&);
};

int main() {
  using Result =
      decltype(std::declval<Container<Policy<int>>&>().get(1));
  static_assert(std::is_same<Result, int&>::value, "");
  return 0;
}
