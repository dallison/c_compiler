// RUN: -std=c++17
//
// A declaration in the target namespace extends an overload set introduced
// by a using-declaration; lookup must retain both the imported function and the
// new function template.

struct wrapped {};

int convert(double) { return 0; }

namespace example {
using ::convert;

template <class T>
T convert(const T& value) {
  return value;
}
}

void use_extended_overload_set() {
  int scalar = example::convert(1.0);
  wrapped value = example::convert(wrapped());
  (void)scalar;
  (void)value;
}
