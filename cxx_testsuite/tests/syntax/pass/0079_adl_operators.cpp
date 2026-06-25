// RUN: -std=c++17
namespace adl_ops {
struct Number {
  int value;
};

int operator+(Number left, Number right) {
  return left.value + right.value;
}

int operator-(Number value) {
  return -value.value;
}

int operator==(Number left, Number right) {
  return left.value == right.value;
}
}

namespace adl_left {
struct Left {
  int value;
};
}

namespace adl_right {
struct Right {
  int value;
};

int operator*(adl_left::Left left, Right right) {
  return left.value * right.value;
}
}

int main(void) {
  adl_ops::Number one = {1};
  adl_ops::Number two = {2};
  adl_left::Left left = {3};
  adl_right::Right right = {4};
  int selected_binary = one + two;
  int selected_unary = -one;
  int selected_comparison = one == two;
  int selected_mixed = left * right;
  return selected_binary + selected_unary + selected_comparison +
         selected_mixed;
}
