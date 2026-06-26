// RUN: -std=c++20
// EXPECT: sizeof... requires a parameter pack

template <class T>
int bad_sizeof_pack(T value) {
  (void)value;
  return sizeof...(T);
}
