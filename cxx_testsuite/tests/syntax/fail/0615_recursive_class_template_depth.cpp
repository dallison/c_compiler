// RUN: -std=c++20
// EXPECT: recursive template instantiation exceeded maximum depth of 1024

template <int N> struct R {
  static const int v = R<N + 1>::v;
};

int main() { return R<0>::v; }
