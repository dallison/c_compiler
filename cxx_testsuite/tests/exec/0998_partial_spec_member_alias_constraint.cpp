// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class T>
struct Impl;

template <class R, class... P>
struct Impl<R(P...) noexcept(false)> {
  template <class F>
  using CallIsValid = std::bool_constant<
      std::disjunction<std::is_invocable_r<R, std::decay_t<F>&, P...>,
                       std::is_same<R, std::invoke_result_t<std::decay_t<F>&,
                                                            P...>>>::value>;
};

template <class Sig>
struct Box {
  template <class F, typename = std::enable_if_t<
                         Impl<Sig>::template CallIsValid<F>::value>>
  Box(F&& fn) : value(fn()) {}
  int value;
};

int main() {
  Box<int()> box([] { return 7; });
  return box.value == 7 ? 0 : 1;
}
