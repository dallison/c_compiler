// RUN: -std=c++20
// A friend declaration must name a class type or a function.
// EXPECT: friend declaration does not name a class or a function

struct Bad {
  // 'friend int;' names neither a class nor a function.
  friend int;
};

int main(void) {
  Bad b;
  (void)b;
  return 0;
}
