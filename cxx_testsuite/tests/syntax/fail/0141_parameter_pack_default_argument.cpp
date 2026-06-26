// RUN: -std=c++20
// EXPECT: function parameter pack cannot have a default argument
template <class... Ts>
int bad_pack_default(Ts... values = 0) {
  return 0;
}
