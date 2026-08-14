// RUN: -std=c++26

constexpr int rethrow_to_outer_handler() {
  try {
    try {
      throw 31;
    } catch (int) {
      throw;
    }
  } catch (int value) {
    return value;
  }
  return 0;
}

static_assert(rethrow_to_outer_handler() == 31);
