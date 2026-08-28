// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// A loop-invariant value that a call inside the loop passed as an argument was
// read before it was computed, on x86-64 at -O2.
//
// Loop-invariant code motion needs a preheader to hoist into, and the pass that
// creates one spliced its instructions in front of the loop header's label.  A
// loop whose test sits at the bottom keeps that header below its body, so the
// preheader landed after the body instead of ahead of the loop: the body fell
// into it on every iteration, which cost the hoisting its whole purpose, and
// the lowering, which walks the instruction list in order, reached the body's
// uses of the hoisted values before their definitions and asserted.
//
// Destroying a vector of a type with a destructor is what reached it here.
// vector::__destroy_range walks the elements with the test at the bottom and
// calls a destroy helper per element, so the receiver and the loop bound are
// both invariant and both get hoisted, and the per-element call needs the
// receiver as an argument inside the loop.  Any vector of such a type would not
// compile at all.

#include <vector>

int g_destroyed = 0;

struct Counted {
  int value;
  explicit Counted(int v) : value(v) {}
  ~Counted() { g_destroyed++; }
};

int main() {
  int sum = 0;
  {
    std::vector<Counted> v;
    v.emplace_back(1);
    v.emplace_back(2);
    v.emplace_back(4);
    v.emplace_back(8);
    for (const Counted& c : v) {
      sum += c.value;
    }
  }
  if (sum != 15) {
    return 1;
  }
  if (g_destroyed < 4) {
    return 2;
  }
  return 0;
}
