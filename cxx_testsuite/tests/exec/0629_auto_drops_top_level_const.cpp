// RUN: -std=c++20
// EXPECT_EXIT: 0

// Plain `auto` deduces like a by-value parameter: `auto r = *this;` in a
// const member function is a mutable copy.
template <bool C>
struct It {
  long i;
  It& add(long n) {
    i += n;
    return *this;
  }
  It plus(long n) const {
    auto r = *this;
    r.add(n);
    return r;
  }
};

struct P {
  long i;
  P& add(long n) {
    i += n;
    return *this;
  }
  P plus(long n) const {
    auto r = *this;
    r.add(n);
    return r;
  }
};

template <class A, class B>
struct same {
  static constexpr bool value = false;
};
template <class A>
struct same<A, A> {
  static constexpr bool value = true;
};

int main() {
  const int ci = 5;
  auto copy = ci;
  copy += 1;
  static_assert(same<decltype(copy), int>::value);
  const int arr[2] = {1, 2};
  auto p = arr;
  static_assert(same<decltype(p), const int*>::value);
  auto [first, second] = arr;
  if (first + second != 3 || &first == &arr[0]) {
    return 2;
  }
  It<false> it{0};
  P q{0};
  return it.plus(10).i + q.plus(5).i + copy == 21 ? 0 : 1;
}
