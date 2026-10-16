// Member-initializers of a constructor template in a class template see both
// the class's arguments and the constructor's own: class pack expansions
// (`c(A()...)`, `c(a...)`, `static_cast<A>(0)...`), types naming the
// constructor's parameter (`W<C>{c}`, `sizeof(W<C>)`), and a delegating
// constructor whose target is deduced from such a type.  This is the shape of
// googletest's `OnceAction`.
#include <type_traits>
#include <utility>

template <class F> struct W { F f; };
template <class F> struct Ignore {
  F f;
  int operator()(int) { return f() + 100; }
};

template <class... A> struct Expand {
  int v = 0;
  template <class C> Expand(C c, int) : v(c(A()...)) {}
  template <class C> Expand(C c, long) : v(c(A{}...)) {}
  template <class C> Expand(C c, char) : v(c(static_cast<A>(1)...)) {}
  template <class C> Expand(C c, A... a) : v(c(a...)) {}
};

template <class R> struct Types {
  int v;
  int size;
  template <class C>
  Types(C c) : v(static_cast<int>(W<C>{c}.f)), size(sizeof(W<C>)) {}
};

template <class Sig> struct Once;
template <class R, class... A> struct Once<R(A...)> {
  int v;
  template <class C,
            typename std::enable_if<
                std::is_same<decltype(std::declval<C&>()(std::declval<A>()...)),
                             R>::value,
                int>::type = 0>
  Once(C&& c) : v(c(A()...)) {}
  template <class C,
            typename std::enable_if<
                std::is_same<decltype(std::declval<C&>()()), R>::value &&
                    sizeof...(A) != 0,
                int>::type = 0>
  Once(C&& c)
      : Once(Ignore<typename std::decay<C>::type>{std::forward<C>(c)}) {}
};

struct Sum {
  int operator()(int a, long b) { return a + (int)b; }
};
struct Five {
  int operator()() { return 5; }
};

int main() {
  if (Expand<int, long>(Sum{}, 0).v != 0) return 1;
  if (Expand<int, long>(Sum{}, 0L).v != 0) return 2;
  if (Expand<int, long>(Sum{}, 'x').v != 2) return 3;
  if (Expand<int, long>(Sum{}, 3, 4L).v != 7) return 4;
  Types<char> t(7.5);
  if (t.v != 7 || t.size != (int)sizeof(double)) return 5;
  Once<int(int)> o(Five{});
  if (o.v != 105) return 6;
  return 0;
}
