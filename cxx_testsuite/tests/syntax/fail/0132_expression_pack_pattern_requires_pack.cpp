// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

int accepts_ints(int a, int b, int c);

template <class T>
int bad_expression_pack_pattern(T value) {
  return accepts_ints((value + 1)...);
}

int use_bad_expression_pack_pattern(void) {
  return bad_expression_pack_pattern(1);
}
