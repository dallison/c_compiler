// RUN: -std=c++17
// EXPECT: Invalid type for member 'type' in this instantiation

// At<3, char, int> recurses to At<1>, which has no `type`: the chain names no
// type, so naming it is an error rather than a silent placeholder.
template <int I, class... T> struct At;
template <class H, class... T> struct At<0, H, T...> { using type = H; };
template <int I, class H, class... T> struct At<I, H, T...> {
  using type = typename At<I - 1, T...>::type;
};

int main() {
  typename At<3, char, int>::type x = 0;
  return x;
}
