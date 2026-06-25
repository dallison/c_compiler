// RUN: -std=c++17
// EXPECT: bypassing a variable initialization

struct Box {
  int value;
  Box() : value(1) {
  }
};

int goto_bypasses_constructor(int pick) {
  if (pick) {
    goto target;
  }
  Box box;
target:
  return 0;
}

int goto_bypasses_scalar_initializer(int pick) {
  if (pick) {
    goto target;
  }
  int value = 1;
target:
  return value;
}
