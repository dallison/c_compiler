// RUN: -std=c++20
// EXPECT_EXIT: 0

int sequence[8];
int count;

struct Tracer {
  int value;

  explicit Tracer(int v) : value(v) {}
  ~Tracer() {
    sequence[count++] = value;
  }
};

struct Object {
  Tracer first;
  Tracer second;

  explicit Object(int) : first(1), second(2) {}
  Object() : Object(0) {
    throw 42;
  }
  ~Object() {
    sequence[count++] = 99;
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
  if (count != 3) {
    return 3;
  }
  if (sequence[0] != 99 || sequence[1] != 2 || sequence[2] != 1) {
    return 4;
  }
  return 0;
}
