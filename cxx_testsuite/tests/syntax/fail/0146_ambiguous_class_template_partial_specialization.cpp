// RUN: -std=c++20
// EXPECT: Ambiguous class template partial specialization for AmbiguousPartial

template <class T, class U>
struct AmbiguousPartial {
  int value;
};

template <class T>
struct AmbiguousPartial<T, int> {
  T left;
};

template <class U>
struct AmbiguousPartial<int, U> {
  U right;
};

AmbiguousPartial<int, int> ambiguous;
