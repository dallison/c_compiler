// RUN: -std=c++20
// EXPECT: placeholder type requires exactly one initializer expression

// [expr.new]/2: `auto` in a new-type-id is deduced from a single initializer
// expression.
int main() {
  auto* p = new auto{1, 2};
  return 0;
}
