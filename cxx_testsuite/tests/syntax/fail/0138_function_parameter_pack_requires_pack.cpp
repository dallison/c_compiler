// RUN: -std=c++20
// EXPECT: function parameter pack requires a template parameter pack

template <class T>
int bad_parameter_pack(T... values) {
  return 0;
}
