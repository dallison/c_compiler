// RUN: -std=c++20
// EXPECT_EXIT: 0

int live_ranges;
int destroyed_ranges;

struct tracked_range {
  int value;

  tracked_range() : value(42) {
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
    return &value;
  }

  int* end() {
    return &value + 1;
  }
};

tracked_range make_range() {
  return tracked_range();
}

int main() {
  for (int value : make_range()) {
    if (value != 42 || live_ranges != 1 || destroyed_ranges != 0) {
      return 1;
    }
  }
  if (live_ranges != 0 || destroyed_ranges != 1) {
    return 2;
  }

  for (int value : make_range().view()) {
    if (value != 42 || live_ranges != 0 || destroyed_ranges != 2) {
      return 3;
    }
  }
  if (live_ranges != 0 || destroyed_ranges != 2) {
    return 4;
  }
  return 0;
}
