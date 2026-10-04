// RUN: -std=c++20
// EXPECT_EXIT: 0

// Instantiating `closure<int>` must not resolve the member template's local
// `decltype(all(declv<R>()))` with the class argument bound to `R`: that
// probe used to leave `fn::operator()(T (&)[N])` unable to construct
// `box<int[4]>` later.
template <class T>
T&& declv();

template <class R>
struct box {
  R* p;
  constexpr box(R& r) : p(&r) {}
};

struct fn {
  template <class T, unsigned long N>
  constexpr auto operator()(T (&r)[N]) const {
    return box<T[N]>(r);
  }
  template <class R>
  constexpr box<R> operator()(R& r) const {
    return box<R>(r);
  }
};
inline constexpr fn all{};

template <class D>
struct closure {
  D extra;
  template <class R>
  constexpr auto operator()(R&& r) const {
    using V = decltype(all(declv<R>()));
    V view{all(static_cast<R&&>(r))};
    return view;
  }
};

struct counter {
  int value;
};

int main() {
  closure<int> c{1};
  counter k{5};
  auto kb = c(k);
  int a[] = {1, 2, 3, 4};
  auto b = all(a);
  auto ab = c(a);
  return kb.p->value == 5 && (*b.p)[2] == 3 && (*ab.p)[3] == 4 ? 0 : 1;
}
