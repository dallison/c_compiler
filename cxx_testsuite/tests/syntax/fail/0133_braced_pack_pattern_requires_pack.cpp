// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

template <class T>
int bad_braced_pack_pattern(T value) {
  int values[] = { (value + 1)... };
  return values[0];
}

int use_bad_braced_pack_pattern(void) {
  return bad_braced_pack_pattern(1);
}
