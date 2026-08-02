// RUN: -std=c++23
// EXPECT: Rvalue reference return value must not be an lvalue

struct object {};

object&& function(object& value) {
  return value;
}
