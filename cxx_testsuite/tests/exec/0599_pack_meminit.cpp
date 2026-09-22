// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T, int I>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  explicit constexpr Storage(T v) : value(v) {}
};

template <int... I>
struct IndexSeq {};

template <class Seq, class... Ts>
struct PackImpl;

template <int... I, class... Ts>
struct PackImpl<IndexSeq<I...>, Ts...> : Storage<Ts, I>... {
  PackImpl() : Storage<Ts, I>()... {}
  template <class... Vs>
  PackImpl(Vs... args) : Storage<Ts, I>(args)... {}
};

int main() {
  PackImpl<IndexSeq<0>, int> one(3);
  Storage<int, 0>* only = &one;
  if (only->value != 3) {
    return 1;
  }

  PackImpl<IndexSeq<0, 1>, int, unsigned> two(5, 7u);
  Storage<int, 0>* first = &two;
  Storage<unsigned, 1>* second = &two;
  return first->value == 5 && second->value == 7 ? 0 : 2;
}
