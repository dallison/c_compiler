// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_nested_large
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept NestedLarge was not satisfied [with T = char]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this nested requirement was not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
concept NestedLarge = requires {
  requires Large<T>;
};

template <typename T>
requires NestedLarge<T>
int needs_nested_large(T value) {
  return sizeof(value);
}

int main(void) {
  return needs_nested_large((char)1);
}
