// RUN: -std=c++20
// EXPECT: Numeric literal operator template must have the form template<char...> operator""suffix()

template<int... Values>
int operator""_bad() {
  return sizeof...(Values);
}
