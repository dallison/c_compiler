// RUN: -std=c++20
// EXPECT: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
requires (Large<T>)
int pick(T value) {
  return sizeof(value);
}

int main(void) {
  return pick((char)0);
}
