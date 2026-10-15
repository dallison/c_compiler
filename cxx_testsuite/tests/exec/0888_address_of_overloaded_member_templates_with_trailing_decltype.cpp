// Taking the address of an overloaded function template selects the overload
// whose trailing `decltype` substitutes; the others are discarded.  A member
// template of a class template mixes the class's parameter with its own in
// that `decltype` (gmock's `MatcherBase<T>::MatchAndExplainImpl<P>`).
#include <ostream>
#include <sstream>

struct A {
  int a(int) const { return 1; }
};
struct B {
  int b(int) const { return 2; }
};

template <typename P>
auto Free(const P& p, int v) -> decltype(p.a(v)) {
  return p.a(v);
}
template <typename P>
auto Free(const P& p, int v) -> decltype(p.b(v)) {
  return p.b(v);
}

struct Plain {
  template <typename P>
  static auto f(const P& p, int v) -> decltype(p.a(v)) {
    return p.a(v);
  }
  template <typename P>
  static auto f(const P& p, int v) -> decltype(p.b(v)) {
    return p.b(v);
  }
};

template <typename T>
struct Mixed {
  template <typename P>
  static auto f(const P& p, T v) -> decltype(p.a(v)) {
    return p.a(v);
  }
  template <typename P>
  static auto f(const P& p, T v) -> decltype(p.b(v)) {
    return p.b(v);
  }
  template <typename P>
  static int (*get())(const P&, T) {
    return &f<P>;
  }
};

struct Listener {
  std::ostream* os;
  std::ostream* stream() { return os; }
};

template <typename T>
class MatcherBase {
 public:
  struct VTable {
    bool (*match_and_explain)(const MatcherBase&, const T&, Listener*);
  };
  template <typename P>
  static auto MatchAndExplainImpl(const MatcherBase& m, const T& value,
                                  Listener* listener)
      -> decltype(P::Get(m).MatchAndExplain(value, listener->stream())) {
    return P::Get(m).MatchAndExplain(value, listener->stream());
  }
  template <typename P>
  static auto MatchAndExplainImpl(const MatcherBase& m, const T& value,
                                  Listener* listener)
      -> decltype(P::Get(m).MatchAndExplain(value, listener)) {
    return P::Get(m).MatchAndExplain(value, listener);
  }
  template <typename P>
  const VTable* GetVTable() {
    static constexpr VTable kVTable = {&MatchAndExplainImpl<P>};
    return &kVTable;
  }
  int payload = 0;
};

struct EqStream {
  int want;
  bool MatchAndExplain(int v, std::ostream*) const { return v == want; }
};
struct EqListener {
  int want;
  bool MatchAndExplain(int v, Listener*) const { return v == want + 1; }
};
template <typename M>
struct Policy {
  static const M& Get(const MatcherBase<int>& m) {
    static M impl{m.payload};
    return impl;
  }
};

int main() {
  int (*free_a)(const A&, int) = &Free<A>;
  int (*free_b)(const B&, int) = &Free<B>;
  if (free_a(A{}, 0) != 1 || free_b(B{}, 0) != 2) return 1;

  int (*plain_a)(const A&, int) = &Plain::f<A>;
  int (*plain_b)(const B&, int) = &Plain::f<B>;
  if (plain_a(A{}, 0) != 1 || plain_b(B{}, 0) != 2) return 2;

  if (Mixed<int>::f(A{}, 0) != 1 || Mixed<int>::f<B>(B{}, 0) != 2) return 3;
  if (Mixed<int>::get<A>()(A{}, 0) != 1 || Mixed<int>::get<B>()(B{}, 0) != 2)
    return 4;

  MatcherBase<int> base;
  base.payload = 3;
  std::ostringstream os;
  Listener listener{&os};
  if (!base.GetVTable<Policy<EqStream>>()->match_and_explain(base, 3,
                                                             &listener))
    return 5;
  if (!base.GetVTable<Policy<EqListener>>()->match_and_explain(base, 4,
                                                               &listener))
    return 6;
  return 0;
}
