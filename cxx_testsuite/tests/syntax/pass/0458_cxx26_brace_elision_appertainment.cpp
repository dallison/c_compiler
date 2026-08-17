// RUN: -std=c++26

struct Element {
  int first;
  int second;
  int third;
};

void check_local_array_bound() {
  Element values[] = {1, 2, 3, 4, 5, 6};
  static_assert(sizeof(values) / sizeof(values[0]) == 2);
}

struct Convertible {
  int value;
  operator int() const {
    return value;
  }
};

struct Aggregate {
  Convertible first;
  Convertible second;
  int trailing;
};

void check_conversion_before_elision() {
  Convertible source{7};
  Aggregate converted = {4, source, source};
  (void)converted;
}
