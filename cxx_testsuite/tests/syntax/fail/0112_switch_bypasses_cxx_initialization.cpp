// RUN: -std=c++17
// EXPECT: bypassing a variable initialization

struct Box {
  int value;
  Box() : value(2) {
  }
};

int switch_bypasses_constructor(int pick) {
  switch (pick) {
    Box box;
    case 1:
      return 1;
    default:
      return 0;
  }
}

int switch_bypasses_scalar_initializer(int pick) {
  switch (pick) {
    int value = 3;
    case 1:
      return value;
    default:
      return 0;
  }
}
