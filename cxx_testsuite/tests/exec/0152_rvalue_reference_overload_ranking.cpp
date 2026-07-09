// RUN: -std=c++20
// EXPECT_EXIT: 0

int pick(int&&) {
  return 1;
}

int pick(const int&&) {
  return 2;
}

int pick_lvalue(int&) {
  return 3;
}

int pick_lvalue(const int&) {
  return 4;
}

int main(void) {
  if (pick(1) != 1) {
    return 1;
  }

  const int c = 2;
  if (pick(static_cast<const int&&>(c)) != 2) {
    return 2;
  }

  int value = 3;
  if (pick_lvalue(value) != 3) {
    return 3;
  }

  return 0;
}
