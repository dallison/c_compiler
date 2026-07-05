// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template only_large_param
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

template <Large T>
int only_large_param(T value) {
  return sizeof(value);
}

int main(void) {
  return only_large_param((char)1);
}
