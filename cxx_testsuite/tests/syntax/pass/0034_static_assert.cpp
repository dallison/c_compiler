// RUN: -std=c++17
static_assert(1, "file scope");
static_assert(sizeof(int) >= 2);

namespace checks {
static_assert(2 + 2 == 4, "namespace scope");
}

struct Box {
  static_assert(sizeof(int) == sizeof(int), "class scope");
  int value;
};

int main(void) {
  static_assert(sizeof(Box) >= sizeof(int), "block scope");
  return 0;
}
