// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template only_large
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
requires Large<T>
int only_large(T value) {
  return 1;
}

int main(void) {
  return only_large('x');
}
