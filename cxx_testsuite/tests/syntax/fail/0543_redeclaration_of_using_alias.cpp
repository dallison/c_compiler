// RUN: -std=c++17
// EXPECT: Symbol tag redeclared with different type
// EXPECT: Symbol value redeclared with different type
// A name introduced by a using-declaration is an alias for another entity and
// carries no type of its own, so an incompatible redeclaration of it must be
// diagnosed at file and at block scope rather than crash.
namespace source {
struct tag;
int value;
}

using source::tag;
void tag(int x) {}

void shadow() {
  using source::value;
  int value = 3;
  (void)value;
}
