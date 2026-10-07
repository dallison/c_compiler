// RUN: -std=c++17
// EXPECT_EXIT: 0
// A namespace-scope constexpr object whose constructor stores `this` compares
// equal to its own address, both when folded and at run time.

struct G {
  const G* self;
  int v = 3;
  constexpr G() : self(this) {}
};

constexpr G g;

static_assert(g.self == &g, "self pointer");

const G* volatile observed = &g;

int main() {
  if (g.self != &g) {
    return 1;
  }
  if (observed->self != observed) {
    return 2;
  }
  return 0;
}
