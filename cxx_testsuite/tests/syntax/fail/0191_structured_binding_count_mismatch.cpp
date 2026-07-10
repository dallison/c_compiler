// RUN: -std=c++20
// EXPECT: Structured binding declaration has wrong number of names
struct Pair {
  int first;
  int second;
};

int value(void) {
  Pair pair = {1, 2};
  auto [one, two, three] = pair;
  return one + two + three;
}
