// RUN: -std=c++20
// EXPECT: Template argument deduction failed

template <class T, class U>
struct pair_template {};

template <template <class> class C, class T>
int read(C<T>) {
  return 1;
}

int use() {
  return read(pair_template<int, long>{});
}
