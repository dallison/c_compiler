// RUN: -std=c++20
// EXPECT: Template instantiation is not supported yet

template <typename T>
T identity(T value);

int main(void) {
  return identity<int>(1);
}
