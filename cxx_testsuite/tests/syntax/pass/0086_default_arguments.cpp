// RUN: -std=c++20
int default_value(void) {
  return 4;
}

int add_defaults(int left, int right = 2, int extra = default_value()) {
  return left + right + extra;
}

int declared_default(int value, int extra = 9);

int declared_default(int value, int extra) {
  return value + extra;
}

int overloaded_default(int value, int extra = 3) {
  return value + extra;
}

int overloaded_default(int value, long extra) {
  return value + (int)extra + 10;
}

template <class T>
T template_default(T value, int extra = 6) {
  return value + extra;
}

template <class T>
T dependent_template_default(T value, T fallback = T()) {
  return value + fallback;
}

class Counter {
 public:
  int add(int base, int delta = 5);
};

class DeclaredCounter {
 public:
  int add(int base, int delta = 4);
};

int Counter::add(int base, int delta) {
  return base + delta;
}

int DeclaredCounter::add(int base, int delta) {
  return base + delta;
}

int use_default_arguments(void) {
  Counter counter;
  DeclaredCounter declared_counter;
  int a = add_defaults(1);
  int b = add_defaults(1, 3);
  int c = add_defaults(1, 3, 5);
  int declared = declared_default(1);
  int d = overloaded_default(4);
  int e = overloaded_default(4, 5L);
  int template_value = template_default(5);
  int dependent_template_value = dependent_template_default(5);
  int f = counter.add(7);
  int g = counter.add(7, 8);
  int h = declared_counter.add(7);
  return a + b + c + declared + d + e + template_value +
         dependent_template_value + f + g + h;
}
