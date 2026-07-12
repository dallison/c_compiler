// RUN: -std=c++20

extern "C" int atexit(void (*)(void));
extern "C" void abort();

static int scalar_calls;
static int object_constructions;
static int object_destructions;
static int referred = 5;

int next_scalar() {
  return ++scalar_calls;
}

struct PortableBox {
  PortableBox() : value(++object_constructions) {}
  ~PortableBox() { ++object_destructions; }
  int value;
};

void check_destructions() {
  if (object_destructions != 3) {
    abort();
  }
}

int scalar() {
  static int value = next_scalar();
  return value;
}

int& reference() {
  static int& value = referred;
  return value;
}

PortableBox& object() {
  static PortableBox value;
  return value;
}

PortableBox* array() {
  static PortableBox values[2];
  return values;
}

int main() {
  if (atexit(check_destructions) != 0) {
    return 5;
  }
  if (scalar() != 1 || scalar() != 1 || scalar_calls != 1) {
    return 1;
  }
  reference() = 12;
  if (referred != 12 || &reference() != &referred) {
    return 2;
  }
  if (object().value != 1 || object().value != 1 ||
      object_constructions != 1) {
    return 3;
  }
  PortableBox* values = array();
  if (values[0].value != 2 || values[1].value != 3 ||
      array() != values || object_constructions != 3) {
    return 4;
  }
  return 0;
}
