// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_local_member
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept HasLocalMember was not satisfied [with T = int]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this compound requirement expression is invalid
template <typename T>
concept HasLocalMember = requires(T value) { { value.missing }; };

template <typename T>
requires HasLocalMember<T>
int needs_local_member(T value) {
  return value;
}

int main(void) {
  return needs_local_member(1);
}
