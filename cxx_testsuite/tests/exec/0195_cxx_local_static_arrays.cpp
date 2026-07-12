// RUN: -std=c++20

int putchar(int) asm("putchar");
int atexit(void (*)(void)) asm("atexit");

static int constructed;
static int scalar_calls;
static int throwing_attempts;
static int throwing_live;

int next_scalar() {
  return ++scalar_calls;
}

struct Element {
  Element() : number(++constructed) {}
  ~Element() { putchar('0' + number); }
  int number;
};

struct ThrowingElement {
  ThrowingElement() {
    if (++throwing_attempts == 2) {
      throw 9;
    }
    ++throwing_live;
  }
  ~ThrowingElement() { --throwing_live; }
};

Element* objects() {
  static Element values[3];
  return values;
}

int* scalars() {
  static int values[2] = {next_scalar(), next_scalar()};
  return values;
}

ThrowingElement* throwing_objects() {
  static ThrowingElement values[3];
  return values;
}

int multidimensional_objects() {
  static Element values[2][2];
  return values[0][0].number + values[0][1].number +
         values[1][0].number + values[1][1].number;
}

void newline_at_exit() {
  putchar('\n');
}

int main() {
  if (atexit(newline_at_exit) != 0) {
    return 1;
  }
  Element* first = objects();
  if (constructed != 3 || objects() != first) {
    return 2;
  }
  int* values = scalars();
  if (values[0] != 1 || values[1] != 2 || scalar_calls != 2) {
    return 3;
  }
  if (scalars() != values || scalar_calls != 2) {
    return 4;
  }
  try {
    throwing_objects();
    return 5;
  } catch (int value) {
    if (value != 9 || throwing_live != 0 || throwing_attempts != 2) {
      return 6;
    }
  }
  ThrowingElement* throwing = throwing_objects();
  if (throwing == nullptr || throwing_live != 3 || throwing_attempts != 5) {
    return 7;
  }
  if (multidimensional_objects() != 22 ||
      multidimensional_objects() != 22 || constructed != 7) {
    return 8;
  }
  return 0;
}
