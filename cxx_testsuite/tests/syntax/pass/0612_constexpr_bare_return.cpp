// RUN: -std=c++20
// A return statement without an operand ends a constexpr void function.
constexpr void early(int) { return; }
constexpr bool free_call() {
  early(1);
  return true;
}
static_assert(free_call());

constexpr void clamp(int& v, int hi) {
  if (v <= hi) {
    return;
  }
  v = hi;
}
constexpr int clamped(int v) {
  clamp(v, 10);
  return v;
}
static_assert(clamped(4) == 4);
static_assert(clamped(40) == 10);

template <class T>
struct Counter {
  T n = 0;
  constexpr void reserve(unsigned long need) {
    if (need < 16) {
      return;
    }
    n = -1;
  }
  constexpr void push() {
    reserve(n + 1);
    n++;
  }
};
constexpr bool member_call() {
  Counter<int> c;
  c.push();
  c.push();
  return c.n == 2;
}
static_assert(member_call());

int main() { return 0; }
