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

int main(void) {
  Counter counter;
  DeclaredCounter declared_counter;
  if (add_defaults(1) != 7) {
    return 1;
  }
  if (add_defaults(1, 3) != 8) {
    return 2;
  }
  if (add_defaults(1, 3, 5) != 9) {
    return 3;
  }
  if (declared_default(1) != 10) {
    return 4;
  }
  if (overloaded_default(4) != 7) {
    return 5;
  }
  if (overloaded_default(4, 5L) != 19) {
    return 6;
  }
  if (template_default(5) != 11) {
    return 7;
  }
  if (dependent_template_default(5) != 5) {
    return 8;
  }
  if (counter.add(7) != 12) {
    return 9;
  }
  if (counter.add(7, 8) != 15) {
    return 10;
  }
  if (declared_counter.add(7) != 11) {
    return 11;
  }
  return 0;
}
