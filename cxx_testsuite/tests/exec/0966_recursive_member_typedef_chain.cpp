// RUN: -std=c++17
// EXPECT_EXIT: 0
// Member typedefs that name another specialization's member typedef resolve
// through the whole chain when named from non-template code, with and without
// `typename`.
template <int I, class... T> struct At;
template <class H, class... T> struct At<0, H, T...> { using type = H; };
template <int I, class H, class... T> struct At<I, H, T...> {
  using type = typename At<I - 1, T...>::type;
};
template <int I> struct P { using type = char; };
template <int I> struct Q { using type = typename P<I - 1>::type; };
template <int I, class... T> struct R { using type = typename At<I, T...>::type; };

int main() {
  typename At<2, char, int, long long>::type a = 0;
  At<1, char, short>::type b = 0;
  Q<1>::type c = 0;
  typename Q<5>::type d = 0;
  R<1, int, double>::type e = 0;
  if (sizeof(a) != 8 || sizeof(b) != 2 || sizeof(c) != 1) return 1;
  if (sizeof(d) != 1 || sizeof(e) != 8) return 2;
  return 0;
}
