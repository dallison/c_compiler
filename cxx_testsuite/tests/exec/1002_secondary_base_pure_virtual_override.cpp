// RUN: -std=c++17
// EXPECT_EXIT: 0

// A class that overrides a pure virtual in a non-primary base is not
// abstract, and a call through that base dispatches to the override.

struct Left {
  virtual int rec(int x) = 0;
  virtual ~Left() {}
};

struct Right {
  virtual int rec(int x, int y) = 0;
  virtual ~Right() {}
};

struct Third {
  virtual int tag() = 0;
  virtual ~Third() {}
};

struct Printer : Left, Right, Third {
  int rec(int x) override { return x + 1; }
  int rec(int x, int y) override { return x + y; }
  int tag() override { return 5; }
};

int main() {
  Printer printer;
  Left* left = &printer;
  Right* right = &printer;
  Third* third = &printer;
  if (left->rec(1) != 2) {
    return 1;
  }
  if (right->rec(3, 4) != 7) {
    return 2;
  }
  if (third->tag() != 5) {
    return 3;
  }
  return 0;
}
