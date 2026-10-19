// RUN: -std=c++17
// EXPECT: Invalid type for member 'type' in this instantiation

// Q<1>::type names P<0>::type, and P is never defined.
template <int I> struct P;
template <int I> struct Q { using type = typename P<I - 1>::type; };

int main() {
  Q<1>::type x = 0;
  return x;
}
