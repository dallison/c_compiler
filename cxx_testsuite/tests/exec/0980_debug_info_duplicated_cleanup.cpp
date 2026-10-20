// RUN: -std=c++20 -g -O2
// EXPECT_EXIT: 0
// An inlined destructor runs on more than one exit path, so its blocks are
// emitted more than once; the debug info names only one copy of each.

static int released;

struct Owner {
  int* p;
  ~Owner() {
    if (p != nullptr) {
      int* old = p;
      p = nullptr;
      released += *old;
    }
  }
};

static int Pick(int n) {
  int value = n;
  Owner owner{&value};
  if (n > 10) {
    return 1;
  }
  if (n < 0) {
    return 2;
  }
  return 3;
}

int main() {
  int total = Pick(20) + Pick(-1) + Pick(5);
  if (total != 6) {
    return 1;
  }
  return released == 24 ? 0 : 2;
}
