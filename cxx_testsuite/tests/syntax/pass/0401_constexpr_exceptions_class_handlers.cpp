// RUN: -std=c++26

struct error {
  int code;

  constexpr explicit error(int value) : code(value) {}
};

constexpr int catch_class_by_value() {
  try {
    error value{41};
    throw value;
  } catch (error value) {
    value.code += 1;
    return value.code;
  }
}

constexpr int catch_class_by_reference() {
  try {
    error value{17};
    throw value;
  } catch (const error& value) {
    return value.code;
  }
}

static_assert(catch_class_by_value() == 42);
static_assert(catch_class_by_reference() == 17);
