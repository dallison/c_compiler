// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template constrained_pick
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
requires Large<T>
int constrained_pick(T value) {
  return 2;
}

int main(void) {
  return constrained_pick((char)0);
}
