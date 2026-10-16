// RUN: -std=c++20
// EXPECT: new-expression with a placeholder type requires an initializer

// [expr.new]/2: `auto` in a new-type-id can only be deduced from an
// initializer.
int main() {
  auto* p = new auto;
  return 0;
}
