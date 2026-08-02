// RUN: -std=c++23
// EXPECT_EXIT: 0

int live_ranges;
int destroyed_ranges;

struct tracked_range {
  int values[3];

  tracked_range() {
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    live_ranges++;
  }

  ~tracked_range() {
    live_ranges--;
    destroyed_ranges++;
  }

  tracked_range& view() {
    return *this;
  }

  int* begin() {
    return values;
  }

  int* end() {
    return values + 3;
  }
};

tracked_range make_range() {
  return tracked_range();
}

int leave_from_loop() {
  for (int value : make_range().view()) {
    if (value != 10 || live_ranges != 1) {
      return 1;
    }
    return 0;
  }
  return 2;
}

int main() {
  int sum = 0;
  for (int value : make_range().view()) {
    if (live_ranges != 1) {
      return 3;
    }
    sum += value;
  }
  if (sum != 60) {
    return 4;
  }
  if (live_ranges != 0) {
    return 9;
  }
  if (destroyed_ranges != 1) {
    return 10;
  }

  for (int guard = 42; int value : make_range().view()) {
    if (guard != 42 || value != 10 || live_ranges != 1) {
      return 5;
    }
    break;
  }
  if (live_ranges != 0 || destroyed_ranges != 2) {
    return 6;
  }

  if (leave_from_loop() != 0) {
    return 7;
  }
  if (live_ranges != 0 || destroyed_ranges != 3) {
    return 8;
  }
  return 0;
}
