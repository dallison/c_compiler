// RUN: -std=c++20 -Werror=reorder-ctor-init
// EXPECT: constructor initializer for x does not match declaration order

struct Order {
  int x;
  int y;

  Order(void) : y(2), x(1) {}
};

Order value;
