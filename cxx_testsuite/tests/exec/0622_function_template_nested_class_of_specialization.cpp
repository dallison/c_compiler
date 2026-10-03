// RUN: -std=c++20
// EXPECT_EXIT: 0

// `typename Stream<C, T>::sentry` in a function template whose parameters
// happen to line up with Stream's own (`C` at 0, `T` at 1) names the nested
// class of the specialization, as in iomanip's `operator>>(istream&, quoted)`.
// It is not the current instantiation of Stream.

template <class C, class T>
struct Base {
  int good() const { return g; }
  int g = 1;
};

template <class C, class T>
struct Stream : Base<C, T> {
  class sentry {
    bool ok_;

   public:
    explicit sentry(Stream& s, bool skip = false) : ok_(false) {
      ok_ = s.good() && !skip;
    }
    explicit operator bool() const { return ok_; }
  };

  int check() {
    typename Stream<C, T>::sentry inner(*this);
    return inner ? 3 : 4;
  }
};

template <class C, class T>
int probe(Stream<C, T>& s) {
  typename Stream<C, T>::sentry sentry(s);
  return (bool)sentry ? 1 : 2;
}

template <class T, class C>
int probe_swapped(Stream<C, T>& s) {
  typename Stream<C, T>::sentry sentry(s, true);
  return (bool)sentry ? 1 : 2;
}

int main() {
  Stream<char, int> s;
  if (probe(s) != 1) {
    return 1;
  }
  if (probe_swapped(s) != 2) {
    return 2;
  }
  if (s.check() != 3) {
    return 3;
  }
  s.g = 0;
  if (probe(s) != 2) {
    return 4;
  }
  return 0;
}
