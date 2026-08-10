// RUN: -std=c++23
// EXPECT_EXIT: 0

#define IDENTITY(value) value

struct pair_value {
  int first;
  int second;

  pair_value() : first(7), second(9) {}
  pair_value(int a, int b) : first(a), second(b) {}

  static pair_value make_default();
  static pair_value make_values();
};

pair_value pair_value::make_default() {
  return pair_value();
}

pair_value pair_value::make_values() {
  return IDENTITY(pair_value(11, 13));
}

int main() {
  pair_value first = pair_value::make_default();
  if (first.first != 7 || first.second != 9) return 1;
  pair_value second = pair_value::make_values();
  if (second.first != 11 || second.second != 13) return 2;
  return 0;
}
