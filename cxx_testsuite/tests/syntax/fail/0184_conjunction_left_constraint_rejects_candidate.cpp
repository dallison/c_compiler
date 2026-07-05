// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_large_and_type
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because the left operand of this conjunction constraint was not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
concept HasNestedType = requires { typename T::type; };

template <typename T>
requires Large<T> && HasNestedType<T>
int needs_large_and_type(T value) {
  return sizeof(value);
}

int main(void) {
  return needs_large_and_type((char)0);
}
