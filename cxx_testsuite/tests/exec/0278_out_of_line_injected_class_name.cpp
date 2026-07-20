// RUN: -std=c++20
// EXPECT_EXIT: 0

namespace models {

struct Value {
  int number;

  explicit Value(int input) : number(input) {}
  Value(const Value& other) : number(other.number) {}
  Value& operator=(const Value& other);
};

}  // namespace models

models::Value& models::Value::operator=(const models::Value& other) {
  Value temporary(other);
  number = temporary.number;
  return *this;
}

int main() {
  models::Value left(1);
  models::Value right(42);
  left = right;
  return left.number == 42 ? 0 : 1;
}
