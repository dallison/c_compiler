// RUN: -std=c++17
// EXPECT_EXIT: 0

// Copy-initialization from another class type ([over.match.copy]): a
// converting constructor's argument may not itself use a user-defined
// conversion, so `M m = o;` uses `O::operator M()` rather than converting
// `o` to `initializer_list<int>` for `M(initializer_list<int>)`.

#include <initializer_list>

struct M {
  int n;
  M() : n(1) {}
  M(std::initializer_list<int>) : n(2) {}
  M(const M& o) : n(o.n) {}
};

struct O {
  template <typename C>
  operator C() const {
    return C();
  }
};

struct Source {
  int v;
};

struct N {
  int n;
  N(const Source& s) : n(s.v) {}
  N(const N& o) : n(o.n) {}
};

int main() {
  O o;
  M m = o;
  if (m.n != 1) {
    return m.n;
  }
  Source s{5};
  N n = s;
  if (n.n != 5) {
    return 3;
  }
  return 0;
}
