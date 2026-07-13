// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_nested_bool
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept NestedBool was not satisfied [with T = int]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this nested requirement was not satisfied
// EXPECT: because concept ExactBool was not satisfied [with T = int]
// EXPECT: because this constraint expression is not a prvalue constant expression of type bool
template <typename T>
concept ExactBool = 1;

template <typename T>
concept NestedBool = requires {
  requires ExactBool<T>;
};

template <typename T>
requires NestedBool<T>
int needs_nested_bool(T value) {
  return value;
}

int main(void) {
  return needs_nested_bool(0);
}
