// RUN: -std=c++20
// EXPECT: constraints not satisfied for function template needs_int_sized_result
// EXPECT: candidate template ignored: constraints not satisfied
// EXPECT: because concept HasIntSizedResult was not satisfied [with T = char]
// EXPECT: because this requires-expression was not satisfied
// EXPECT: because this compound requirement return type constraint was not satisfied
// EXPECT: because return type char did not satisfy concept SizeIs [with T = char, N = 4]
template <typename T, int N>
concept SizeIs = sizeof(T) == N;

char returns_char(char value);

template <typename T>
concept HasIntSizedResult = requires(T value) {
  { returns_char(value) } -> SizeIs<sizeof(int)>;
};

template <typename T>
requires HasIntSizedResult<T>
int needs_int_sized_result(T value) {
  return value;
}

int main(void) {
  return needs_int_sized_result((char)1);
}
