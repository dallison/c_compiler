// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_member
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept HasCompoundMember was not satisfied [with T = int]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this compound requirement expression is invalid
template <typename T>
concept HasCompoundMember = requires { { T::missing }; };

template <typename T>
requires HasCompoundMember<T>
int needs_member(T value) {
  return value;
}

int main(void) {
  return needs_member(1);
}
