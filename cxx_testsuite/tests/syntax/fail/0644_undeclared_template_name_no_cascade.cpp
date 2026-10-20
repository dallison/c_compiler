// RUN: -std=c++20
// EXPECT: No template named "Q"
// EXPECT: No such symbol "baz"
// EXPECT-NOT: Expected semicolon
// EXPECT-NOT: Type expected
// An undeclared name followed by a balanced `<...>` is read as a template-id
// ([temp.names]/3), so its `>` does not end an expression.  A `<` whose
// partner `>` lies outside the enclosing parentheses is still a comparison.

template <typename T>
struct B {};

template <typename T>
struct A {
  B<Q<T>> b;
};

int Compare(int a) { return (baz < a) > 0; }

int main() {
  A<int> a;
  return 0;
}
