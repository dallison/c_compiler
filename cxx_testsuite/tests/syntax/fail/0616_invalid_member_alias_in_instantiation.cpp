// RUN: -std=c++17
// EXPECT: Invalid type for member 'type' in this instantiation

// Holder<int>::type is invalid (`int::value` is not a type or value); naming
// it outside a deduction context is an error, not a silent placeholder.
template <class T> struct Holder {
  using type = decltype(T::value);
};

int main() {
  Holder<int>::type x = 0;
  return x;
}
