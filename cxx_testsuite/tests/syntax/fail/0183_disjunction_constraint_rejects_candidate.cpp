// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template accepts_either
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because neither operand of this disjunction constraint was satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because concept HasNestedType was not satisfied [with T = char]
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
concept HasNestedType = requires { typename T::type; };

template <typename T>
requires Large<T> || HasNestedType<T>
int accepts_either(T value) {
  return sizeof(value);
}

int main(void) {
  return accepts_either((char)0);
}
