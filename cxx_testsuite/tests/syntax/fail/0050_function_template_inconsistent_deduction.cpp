// RUN: -std=c++20
// EXPECT: Template instantiation is not supported yet

template <typename T>
T same(T left, T right) {
  return left + right;
}

int main(void) {
  return same(1, 'c');
}
