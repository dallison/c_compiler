// RUN: -std=c++20
// EXPECT: Function template definition is required for instantiation

template <typename T>
T identity(T value);

int main(void) {
  return identity<int>(1);
}
