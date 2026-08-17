// RUN: -std=c++20
// EXPECT_EXIT: 0

int sequence[8];
int count;

struct Element {
  int value;

  explicit Element(int v) : value(v + 100) {
    if (v == 2) {
      throw 42;
    }
  }
  ~Element() {
    sequence[count++] = value;
  }
};

struct Object {
  Element elements[3];

  Object() : elements{0, 1, 2} {}
};

struct CompleteArrayObject {
  Element elements[3];

  CompleteArrayObject() : elements{0, 1, 3} {
    throw 7;
  }
};

int main() {
  try {
    Object object;
    (void)object;
    return 1;
  } catch (int value) {
    if (value != 42) {
      return 2;
    }
  }
  if (count != 2) {
    return 3;
  }
  if (sequence[0] != 101 || sequence[1] != 100) {
    return 4;
  }

  count = 0;
  try {
    CompleteArrayObject object;
    (void)object;
    return 5;
  } catch (int value) {
    if (value != 7) {
      return 6;
    }
  }
  if (count != 3) {
    return 7;
  }
  if (sequence[0] != 103 || sequence[1] != 101 || sequence[2] != 100) {
    return 8;
  }
  return 0;
}
