// RUN: -std=c++20
// EXPECT_EXIT: 0
// C++20 value-dependent explicit(bool): the condition is deferred to template
// instantiation and re-evaluated per specialization.

template <class T>
struct Box {
  int v;
  // Explicit only when sizeof(T) > 2, decided at instantiation time.
  explicit(sizeof(T) > 2) Box(int x) : v(x) {}
};

// sizeof(char) == 1 -> not explicit -> copy-initialization and implicit
// conversion in a call argument are both allowed.
int take_small(Box<char> b) { return b.v; }

int main(void) {
  Box<char> a = 5;      // ok: not explicit for char
  Box<int> b(7);        // direct-init works regardless of explicit-ness
  int r = take_small(3);  // ok: implicit conversion allowed for char

  return (a.v + b.v + r) == 15 ? 0 : 1;  // 5 + 7 + 3 == 15
}
