// RUN: -std=c++20 -fconstexpr-eval=audit

// A local declared without an initializer may be written before it is read in
// a constant expression.  Arrays, classes and locals whose address is taken
// live in memory, so whether a byte was written is only known as evaluation
// runs.
struct Pair {
  int a, b;
};

struct Defaulted {
  int a = 1;
  int b;
};

constexpr int array_filled_in_a_loop() {
  int values[4];
  for (int i = 0; i < 4; i++) {
    values[i] = i * i;
  }
  return values[1] + values[3];
}
static_assert(array_filled_in_a_loop() == 10);

constexpr int members_assigned() {
  Pair pair;
  pair.a = 3;
  pair.b = 4;
  Pair copy = pair;
  return copy.a * copy.b;
}
static_assert(members_assigned() == 12);

constexpr int unwritten_member_not_read() {
  Defaulted value;
  return value.a;
}
static_assert(unwritten_member_not_read() == 1);

constexpr int written_through_pointer() {
  int value;
  int* pointer = &value;
  *pointer = 5;
  return value;
}
static_assert(written_through_pointer() == 5);

constexpr int fresh_each_iteration() {
  int sum = 0;
  for (int i = 0; i < 3; i++) {
    int term;
    term = i + 1;
    sum += term;
  }
  return sum;
}
static_assert(fresh_each_iteration() == 6);

constexpr int factorial(int n) {
  int slots[2];
  if (n == 0) {
    return 1;
  }
  slots[n % 2] = n;
  return factorial(n - 1) * slots[n % 2];
}
static_assert(factorial(4) == 24);
