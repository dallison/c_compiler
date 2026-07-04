// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Tracked {
  static int live;
  int value;

  explicit Tracked(int v) : value(v) {
    ++live;
  }

  Tracked(const Tracked& other) : value(other.value) {
    ++live;
  }

  ~Tracked() {
    --live;
  }
};

int Tracked::live = 0;

int consume(const Tracked& item) {
  return item.value;
}

int main(void) {
  consume(Tracked(7));
  if (Tracked::live != 0) {
    return 1;
  }
  return 0;
}
