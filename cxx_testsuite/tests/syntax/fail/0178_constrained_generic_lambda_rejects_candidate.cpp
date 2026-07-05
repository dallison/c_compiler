// RUN: -std=c++20
// EXPECT: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

int main(void) {
  auto lambda = [](Large auto value) { return sizeof(value); };
  return lambda((char)1);
}
