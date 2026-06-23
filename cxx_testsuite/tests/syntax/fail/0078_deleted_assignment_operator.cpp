// RUN: -std=c++20

struct NoAssign {
  int value;

  NoAssign() = default;
  struct NoAssign& operator=(const NoAssign& other) = delete;
};

int main(void) {
  NoAssign first;
  NoAssign second;
  first = second;
  return 0;
}
