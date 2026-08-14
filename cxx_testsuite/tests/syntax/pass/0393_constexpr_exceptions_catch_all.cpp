// RUN: -std=c++26

constexpr int catch_anything(bool should_throw) {
  try {
    if (should_throw) {
      throw 17;
    }
    return 3;
  } catch (...) {
    return 9;
  }
}

static_assert(catch_anything(false) == 3);
static_assert(catch_anything(true) == 9);
