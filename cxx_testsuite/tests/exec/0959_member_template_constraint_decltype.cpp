// RUN: -std=c++20
// A constructor template's constraint names its own parameter inside
// `decltype` (`decltype(declval<Range&>().data())`).  Instantiating the class
// binds only the class's parameter; the constraint's operand must keep naming
// the constructor's parameter rather than taking the class argument.
template <class T> T&& declval_() noexcept;
template <class A, class B> struct same { static constexpr bool value = false; };
template <class A> struct same<A, A> { static constexpr bool value = true; };

struct Vec {
  int d[3] = {4, 6, 10};
  int* data() { return d; }
  int size() const { return 3; }
};

template <class T> struct Span {
  using pointer = T*;
  T* p;
  int n;
  template <class Range>
    requires(same<decltype(declval_<Range&>().data()), pointer>::value)
  Span(Range& range) : p(range.data()), n(range.size()) {}
};

template <class T> struct Holder {
  template <class U>
    requires(same<decltype(declval_<U&>() + declval_<T&>()), long>::value)
  static int Sum(U u, T t) { return (int)(u + t); }
};

int main() {
  Vec v;
  Span<int> s(v);
  if (s.n != 3 || s.p[1] != 6) return 1;
  if (Holder<int>::Sum(2L, 3) != 5) return 2;
  return 0;
}
