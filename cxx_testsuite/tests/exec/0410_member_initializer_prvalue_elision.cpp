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

struct Value {
  int value;

  explicit Value(int v) : value(v) {}
  Value(const Value& other) : value(other.value) {}
  ~Value() {
    sequence[count++] = 20;
  }
};

struct Object {
  Tracer first;
  Value value;

  Object() : first(1), value(Value(2)) {
    throw 42;
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
  if (sequence[0] != 20 || sequence[1] != 1) {
    return 4;
  }
  return 0;
}
