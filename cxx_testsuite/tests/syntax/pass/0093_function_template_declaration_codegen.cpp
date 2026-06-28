// RUN: -std=c++20

template <typename T>
T identity(T value);

int main(void) {
  return identity<int>(1);
}
