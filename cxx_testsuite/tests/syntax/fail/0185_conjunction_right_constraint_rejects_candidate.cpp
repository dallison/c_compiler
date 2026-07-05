// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_always_and_type
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because the right operand of this conjunction constraint was not satisfied
// EXPECT: because concept HasNestedType was not satisfied [with T = int]
template <typename T>
concept Always = true;

template <typename T>
concept HasNestedType = requires { typename T::type; };

template <typename T>
requires Always<T> && HasNestedType<T>
int needs_always_and_type(T value) {
  return sizeof(value);
}

int main(void) {
  return needs_always_and_type(0);
}
