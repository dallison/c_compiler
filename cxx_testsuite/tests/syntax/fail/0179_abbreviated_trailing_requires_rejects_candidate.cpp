// RUN: -std=c++20
// EXPECT: constraints not satisfied
// EXPECT: because concept Large was not satisfied [with T = char]
// EXPECT: because this constraint expression evaluated to false
template <typename T>
concept Large = sizeof(T) > 4;

long long* pick(auto value) requires Large<decltype(value)> {
  return (long long*)0;
}

int main(void) {
  pick((char)0);
}
