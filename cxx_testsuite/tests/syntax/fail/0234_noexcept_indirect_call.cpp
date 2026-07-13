// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_indirect_noexcept
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept IndirectNoexcept was not satisfied [with T = int]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this compound requirement is not noexcept
int throwing_helper(int value);

int indirect_call(int value) {
  return throwing_helper(value);
}

template <typename T>
concept IndirectNoexcept = requires(T value) {
  { indirect_call(value) } noexcept;
};

template <typename T>
requires IndirectNoexcept<T>
int needs_indirect_noexcept(T value) {
  return value;
}

int main(void) {
  return needs_indirect_noexcept(1);
}
