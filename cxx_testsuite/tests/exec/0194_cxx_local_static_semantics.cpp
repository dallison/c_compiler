// RUN: -std=c++20

int putchar(int) asm("putchar");
int atexit(void (*)(void)) asm("atexit");

static int calls;
static int referred = 10;
static int throw_attempts;
static int lambda_initializations;

int next_value() {
  return ++calls;
}

int dynamic_scalar() {
  static int value = next_value();
  return value;
}

int& dynamic_reference() {
  static int& reference = referred;
  return reference;
}

int throwing_initializer() {
  if (++throw_attempts == 1) {
    throw 7;
  }
  return 44;
}

int retry_after_throw() {
  static int value = throwing_initializer();
  return value;
}

struct Box {
  Box(char value) : tag(value), report(true) {}
  Box(char value, int) : tag(value), report(false) {}
  Box(const Box& other) : tag(other.tag), report(true) {}
  ~Box() {
    if (report) {
      putchar(tag);
    }
  }
  char tag;
  bool report;
};

struct ConstantBox {
  constexpr ConstantBox(char value) : tag(value) {}
  ~ConstantBox() { putchar(tag); }
  char tag;
};

Box& direct_object() {
  static Box object('D');
  return object;
}

Box& list_object() {
  static Box object{'L'};
  return object;
}

Box& copy_object() {
  Box source('C', 0);
  static Box object = source;
  return object;
}

Box& ordered_object(char tag) {
  static Box first(tag);
  return first;
}

Box& second_ordered_object(char tag) {
  static Box second(tag);
  return second;
}

Box& never_called() {
  static Box skipped('X');
  return skipped;
}

ConstantBox& constant_object() {
  static ConstantBox object('K');
  return object;
}

ConstantBox& never_called_constant_object() {
  static ConstantBox skipped('Y');
  return skipped;
}

int lambda_static() {
  auto function = []() {
    static int value = ++lambda_initializations;
    return value;
  };
  return function();
}

void newline_at_exit() {
  putchar('\n');
}

int main() {
  if (atexit(newline_at_exit) != 0) {
    return 6;
  }
  if (dynamic_scalar() != 1 || dynamic_scalar() != 1 || calls != 1) {
    return 1;
  }
  dynamic_reference() = 27;
  if (referred != 27 || &dynamic_reference() != &referred) {
    return 2;
  }
  direct_object();
  list_object();
  copy_object();
  try {
    retry_after_throw();
    return 3;
  } catch (int value) {
    if (value != 7) {
      return 4;
    }
  }
  if (retry_after_throw() != 44 || throw_attempts != 2) {
    return 5;
  }
  ordered_object('A');
  second_ordered_object('B');
  if (constant_object().tag != 'K') {
    return 8;
  }
  if (lambda_static() != 1 || lambda_static() != 1 ||
      lambda_initializations != 1) {
    return 7;
  }
  return 0;
}
