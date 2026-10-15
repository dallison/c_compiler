// `Spy<A>::Base::combine(...)` names the injected-class-name of a dependent
// base, which is the specialization `Base<Spy<A>>` (abseil's
// `SpyHashStateImpl<A>::HashStateBase::combine`).
#include <utility>

template <typename H>
struct Base {
  static H combine(H state) { return state; }
  template <typename T, typename... Ts>
  static H combine(H state, const T& value, const Ts&... values) {
    state.n += value;
    return H::combine(std::move(state), values...);
  }
};

template <typename T>
struct Spy : Base<Spy<T>> {
  int n = 0;
  Spy() = default;
  template <typename U>
  Spy(Spy<U>&& other) : n(other.n) {}

  template <typename A>
  static Spy ThroughDependentSpecialization(Spy s, const A& a) {
    return Spy<A>::Base::combine(std::move(s), a);
  }
  template <typename A>
  static Spy ThroughAlias(Spy s, const A& a) {
    using B = typename Spy<A>::Base;
    return B::combine(std::move(s), a);
  }
  template <typename A>
  static Spy ThroughConcreteSpecialization(Spy s, const A& a) {
    return Spy<int>::Base::combine(std::move(s), a);
  }
  template <typename A>
  static Spy ThroughInjectedClassName(Spy s, const A& a, const A& b) {
    return Spy::Base::combine(std::move(s), a, b);
  }
};

int main() {
  Spy<void> s;
  s = Spy<void>::ThroughDependentSpecialization(std::move(s), 1);
  if (s.n != 1) return 1;
  s = Spy<void>::ThroughAlias(std::move(s), 2);
  if (s.n != 3) return 2;
  s = Spy<void>::ThroughConcreteSpecialization(std::move(s), 3);
  if (s.n != 6) return 3;
  s = Spy<void>::ThroughInjectedClassName(std::move(s), 4, 5);
  if (s.n != 15) return 4;
  Spy<int> t;
  t = Spy<int>::Base::combine(std::move(t), 1, 2);
  return t.n == 3 ? 0 : 5;
}
