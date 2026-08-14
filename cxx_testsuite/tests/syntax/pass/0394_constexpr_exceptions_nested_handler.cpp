// RUN: -std=c++26

constexpr int nested_handler() {
  try {
    try {
      throw 23;
    } catch (long) {
      return 1;
    }
  } catch (int value) {
    return value;
  }
  return 0;
}

static_assert(nested_handler() == 23);
