namespace exec_adl {
struct Box {
  int value;
};

int get(Box box) {
  return box.value + 11;
}

int operator+(Box left, Box right) {
  return left.value + right.value + 13;
}

enum class Color {
  red
};

int paint(Color color) {
  return 17;
}
}

namespace exec_adl_left {
struct Left {
  int value;
};
}

namespace exec_adl_right {
struct Right {
  int value;
};

int mix(exec_adl_left::Left left, Right right) {
  return left.value * 10 + right.value;
}
}

int get(int value) {
  return value;
}

int main(void) {
  exec_adl::Box one = {3};
  exec_adl::Box two = {5};
  exec_adl_left::Left left = {7};
  exec_adl_right::Right right = {9};
  if (get(one) != 14) {
    return 1;
  }
  if (one + two != 21) {
    return 2;
  }
  if (paint(exec_adl::Color::red) != 17) {
    return 3;
  }
  if (mix(left, right) != 79) {
    return 4;
  }
  if (get(23) != 23) {
    return 5;
  }
  return 0;
}
