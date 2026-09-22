// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T, int I>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  explicit constexpr Storage(T v) : value(v) {}
};

template <class T0, class T1>
struct Impl : Storage<T0, 0>, Storage<T1, 1> {
  constexpr Impl() = default;
  constexpr Impl(T0 a, T1 b)
      : Storage<T0, 0>(a), Storage<T1, 1>(b) {}
};

int main() {
  Impl<int, unsigned> x(3, 4u);
  Storage<int, 0>* first = &x;
  Storage<unsigned, 1>* second = &x;
  return first->value == 3 && second->value == 4 ? 0 : 1;
}
