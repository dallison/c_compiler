// RUN: -std=c++17
// EXPECT_EXIT: 0

typedef int Fn(int);

int identity(const int value) { return value; }

struct Box {
  explicit Box(int value);
  explicit Box(int value, Fn* function);
  int value;
};

Box::Box(const int value, Fn* function) : value(function(value)) {}

Box::Box(const int value) : Box(value, identity) {}

int main() {
  Fn* function = identity;
  if (function(6) != 6) return 1;
  Box box(4);
  if (box.value != 4) return 2;
  return 0;
}
