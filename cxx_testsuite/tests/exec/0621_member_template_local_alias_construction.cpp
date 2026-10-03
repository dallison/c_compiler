// RUN: -std=c++20
// EXPECT_EXIT: 0

// A member function template of a class template constructs a value through
// a local alias that depends on both the class and the member parameters
// (`using Result = decltype(f(v)); return Result();`, as in
// optional::and_then).  The alias names a type, not a callable value.

template <class T>
T&& declval();

template <bool B, class A, class C>
struct cond {
  using type = A;
};
template <class A, class C>
struct cond<false, A, C> {
  using type = C;
};
template <bool B, class A, class C>
using cond_t = typename cond<B, A, C>::type;

struct R {
  int x;
  R() : x(7) {}
  R(int v) : x(v) {}
};

R make(int v) { return R(v * 2); }

template <class T>
struct Holder {
  T v;

  template <class F>
  decltype(declval<F>()(declval<T&>())) call(F&& f, bool on) {
    using Result = decltype(declval<F>()(declval<T&>()));
    if (on) {
      return f(v);
    }
    return Result();
  }
};

struct Small {
  int value() const { return 1; }
};
struct Big {
  int value() const { return 2; }
};
struct Gen3 {
  static constexpr int range = 3;
};
struct Gen9 {
  static constexpr int range = 9;
};
template <class G>
constexpr int RangeSize() {
  return G::range;
}

template <class T>
struct Dist {
  T base;

  template <class URBG>
  T gen(URBG&) {
    using tag = cond_t<(RangeSize<URBG>() > 5), Big, Small>;
    return base + tag{}.value();
  }
};

int main() {
  Holder<int> h{3};
  if (h.call(make, true).x != 6) {
    return 1;
  }
  if (h.call(make, false).x != 7) {
    return 2;
  }
  Dist<int> d{10};
  Gen3 g3;
  Gen9 g9;
  if (d.gen(g3) != 11 || d.gen(g9) != 12) {
    return 3;
  }
  return 0;
}
