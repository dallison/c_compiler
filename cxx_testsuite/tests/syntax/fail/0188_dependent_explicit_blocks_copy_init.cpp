// RUN: -std=c++20
// EXPECT: Illegal conversion
// A value-dependent explicit(bool) that evaluates to true at instantiation
// makes the constructor explicit, so copy-initialization must be rejected.

template <class T>
struct Box {
  int v;
  explicit(sizeof(T) > 2) Box(int x) : v(x) {}
};

int main() {
  Box<int> b = 7;  // sizeof(int) > 2 -> explicit -> copy-init is ill-formed
  return b.v;
}
