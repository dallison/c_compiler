// RUN: -std=c++11
// C++ [dcl.init.list]/7: narrowing conversions in scalar list-initialization
// are ill-formed.  A floating-point source converted to an integer target is
// always narrowing; a constant integer source that does not fit in the integer
// target is narrowing.
// EXPECT: narrowing conversion from 'double ' to 'int ' in list-initialization
// EXPECT: narrowing conversion from 'long ' to 'int ' in list-initialization
// EXPECT: narrowing conversion from 'int ' to 'char ' in list-initialization

int main() {
  int a{3.5};
  int b{70000000000};
  char c{300};
  return a + b + c;
}
