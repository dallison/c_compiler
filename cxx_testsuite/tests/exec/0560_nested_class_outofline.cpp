// RUN: -std=c++17
// EXPECT_EXIT: 0

template <typename T, int N>
struct Holder {
  typedef unsigned long size_type;
  struct Nested {
    T value;
    int annotate(size_type n);
  };

  Nested storage;
};

template <typename T, int N>
int Holder<T, N>::Nested::annotate(typename Holder<T, N>::size_type n) {
  return value + (int)n + N;
}

int main() {
  Holder<int, 4> h;
  h.storage.value = 3;
  return h.storage.annotate(5) == 12 ? 0 : 1;
}
