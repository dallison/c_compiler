// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_int_truth
// EXPECT: because concept IntTruth was not satisfied [with T = int]
// EXPECT: because this constraint expression is not a prvalue constant expression of type bool
template <typename T>
concept IntTruth = 1;

template <typename T>
requires IntTruth<T>
int needs_int_truth(T value) {
  return value;
}

int main(void) {
  return needs_int_truth(0);
}
