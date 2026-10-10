// RUN: -std=c++20
// EXPECT_EXIT: 0

// When an element's constructor throws while an automatic array is being
// built, the elements already constructed are destroyed in reverse order
// before the exception leaves the declaration.

static int live;
static int attempts;
static int fail_at;
static int order[8];
static int destroyed;

struct Element {
  int id;
  Element() {
    if (++attempts == fail_at) {
      throw 9;
    }
    id = attempts;
    ++live;
  }
  ~Element() {
    order[destroyed++] = id;
    --live;
  }
};

static void Reset(int fail) {
  live = 0;
  attempts = 0;
  fail_at = fail;
  destroyed = 0;
}

int main() {
  Reset(3);
  try {
    Element row[3];
    return 1;
  } catch (int value) {
    if (value != 9 || live != 0 || destroyed != 2 || order[0] != 2 ||
        order[1] != 1) {
      return 2;
    }
  }

  Reset(4);
  try {
    Element grid[2][2];
    return 3;
  } catch (int) {
    if (live != 0 || destroyed != 3 || order[0] != 3 || order[2] != 1) {
      return 4;
    }
  }

  Reset(0);
  {
    Element pair[2];
    if (live != 2) {
      return 5;
    }
  }
  if (live != 0 || destroyed != 2 || order[0] != 2 || order[1] != 1) {
    return 6;
  }
  return 0;
}
