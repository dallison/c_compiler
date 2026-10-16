// A member template's own pack expanded together with (or around) the
// enclosing class template's pack.  Member parameters are renumbered from 0
// when the class is instantiated, so class packs inside the pattern must be
// substituted first or they collide with the member's parameters.
#include <cstddef>
#include <tuple>
#include <utility>

int take(int a) { return a + 2; }
int take2(int a, char b) { return a + b; }

template <class... A> struct Get {
  template <std::size_t... I>
  int f(std::index_sequence<I...>, const std::tuple<A...>& t) const {
    return take(std::get<I>(t)...);
  }
  template <std::size_t... I>
  int g(std::index_sequence<I...>, const std::tuple<A...>& t) const {
    return take2(std::get<I>(t)...);
  }
};

struct Impl {
  template <class R, class... T> static R Perform(T... a) {
    return (R)(sizeof...(T) * 100 + (0 + ... + (int)a));
  }
};

template <class F> struct Invoker;
template <class R, class... Args> struct Invoker<R(Args...)> {
  using args_type = std::tuple<Args...>;
  R operator()(Args... a) const {
    return Apply(std::make_index_sequence<sizeof...(Args)>{}, args_type{a...});
  }
  template <std::size_t... I>
  R Apply(std::index_sequence<I...>, const args_type& args) const {
    return Impl::template Perform<
        R, typename std::tuple_element<I, args_type>::type...>(
        std::get<I>(args)...);
  }
};

int main() {
  std::tuple<int> t1{5};
  if (Get<int>().f(std::index_sequence<0>{}, t1) != 7) return 1;
  std::tuple<int, char> t2{5, 1};
  if (Get<int, char>().g(std::index_sequence<0, 1>{}, t2) != 6) return 2;
  if (Invoker<int(int)>()(5) != 105) return 3;
  if (Invoker<long(int, char, short)>()(1, 2, 3) != 306) return 4;
  return 0;
}
