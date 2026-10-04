// RUN: -std=c++20
// EXPECT_EXIT: 0

// Inside `O<T>::It<C>`, the injected-class-name `It` names the
// specialization `O<int>::It<false>`, not the member template's pattern.
template <class T>
struct O {
  template <bool C>
  struct It {
    T i;
    It& operator+=(T n) {
      i += n;
      return *this;
    }
    It plus(T n) const {
      It r = *this;
      r += n;
      return r;
    }
    It twice() const { return plus(i); }
  };
};

int main() {
  O<int>::It<false> it{3};
  it = it.plus(10);
  if (it.i != 13) {
    return 1;
  }
  O<long>::It<true> other{4};
  if (other.twice().i != 8) {
    return 2;
  }
  return 0;
}
