// RUN: -std=c++20
struct ExplicitNumber {
  int value;
  explicit operator int() const {
    return value;
  }
};

int main(void) {
  ExplicitNumber number;
  int value = number;
  return value;
}
