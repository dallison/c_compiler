// RUN: -std=c++17
using concept = int;

template <typename T>
concept value;

int main(void) {
  value<int> = 3;
  return value<int>;
}
