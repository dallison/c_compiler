// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_noexcept_call
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept CallsNoexcept was not satisfied [with T = int]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this compound requirement is not noexcept
int maybe_throwing(int value);

template <typename T>
concept CallsNoexcept = requires(T value) {
  { maybe_throwing(value) } noexcept;
};

template <typename T>
requires CallsNoexcept<T>
int needs_noexcept_call(T value) {
  return value;
}

int main(void) {
  return needs_noexcept_call(1);
}
