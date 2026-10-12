// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Pair {
  const void* first;
  int second;

  Pair(const void* first_arg, int second_arg)
      : first(first_arg), second(second_arg) {}
};

struct Base {
  Pair query;
};

struct Derived : Base {};

template <class T>
T make(const void* pointer, int value) {
  return T{{{pointer, value}}};
}

int main() {
  int object = 0;
  Derived result = make<Derived>(&object, 17);
  return result.query.first == &object && result.query.second == 17 ? 0 : 1;
}
