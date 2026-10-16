// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <type_traits>

struct Base {};

template <class X>
struct Box {
  using sender_concept = Base;
  static constexpr int value = 1;
};

template <class T>
struct via_alias {
  using R = std::remove_cvref_t<T>;
  using C = typename R::sender_concept;
};

template <class T>
struct via_direct {
  using C = typename std::remove_cvref_t<T>::sender_concept;
};

int names_base(Base*) { return 1; }
template <class T>
int names_base(T*) { return 0; }

int main() {
  using S = Box<int>;
  if (names_base((typename via_alias<S>::C*)0) != 1) {
    return 1;
  }
  if (names_base((typename via_alias<const S&>::C*)0) != 1) {
    return 2;
  }
  if (names_base((typename via_direct<S>::C*)0) != 1) {
    return 3;
  }
  if (names_base((typename via_direct<const S&>::C*)0) != 1) {
    return 4;
  }
  return 0;
}
