// RUN: -std=c++20
template <class T>
struct wrap {
};

template <class T>
struct holder {
  template <class U>
  using first = wrap<U>;

  template <class U>
  using second = first<U>;
};

using result = holder<int>::second<long>;
