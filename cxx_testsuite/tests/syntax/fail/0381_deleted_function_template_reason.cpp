// RUN: -std=c++26
// EXPECT: this conversion is intentionally unavailable

template <class T>
void convert(T) = delete("this conversion is intentionally unavailable");

int main() {
  convert(3);
  return 0;
}
