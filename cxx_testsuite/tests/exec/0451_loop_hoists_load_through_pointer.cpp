// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0
// A load is annotated with the variable its address came from, which for a
// dereference is the pointer rather than the location read.  Loop-invariant
// code motion used that annotation to decide the load was safe to hoist, so an
// accumulator reached through an unmodified `this` moved out of the loop and
// every iteration added to the value the loop started with.

struct Box {
  int sum;

  void add_all(const int* values, int count) {
    for (int i = 0; i < count; ++i) {
      sum += values[i];
    }
  }
};

static int accumulate_through_pointer(int* total, const int* values,
                                      int count) {
  for (int i = 0; i < count; ++i) {
    *total += values[i];
  }
  return *total;
}

int main(void) {
  int data[4] = {1, 2, 3, 4};

  Box box;
  box.sum = 0;
  box.add_all(data, 4);
  if (box.sum != 10) {
    return 1;
  }

  box.add_all(data, 4);
  if (box.sum != 20) {
    return 2;
  }

  int total = 5;
  if (accumulate_through_pointer(&total, data, 4) != 15) {
    return 3;
  }
  if (total != 15) {
    return 4;
  }

  return 0;
}
