// RUN: -std=c++20
// EXPECT_EXIT: 0

// [dcl.init.ref]/5.3, [expr.static.cast]: the base subobject of a derived
// xvalue (or of a materialized derived prvalue) is an xvalue, so it binds to
// `Base&&` -- including through an inherited constructor
// (flat_hash_set's `raw_hash_set(raw_hash_set&&, const allocator_type&)`).
#include <memory>
#include <utility>

struct Base {
  int v = 1;
  Base() = default;
  explicit Base(int x) : v(x) {}
  Base(Base&& o) noexcept : v(o.v) { o.v = -1; }
  Base(Base&& o, const std::allocator<int>&) : v(o.v + 100) { o.v = -2; }
  Base(const Base& o) : v(o.v) {}
};

struct Derived : Base {
  using Base::Base;
  Derived() = default;
  int w = 2;
};

struct Other {
  int z = 0;
};

struct Second : Other, Base {};

static int Take(Base&& b) { return b.v; }
static int Which(Base&) { return 1; }
static int Which(Base&&) { return 2; }

int main() {
  Derived d(5);
  if (Take(std::move(d)) != 5) return 1;
  if (Take(Derived(6)) != 6) return 2;
  Second s;
  s.v = 7;
  if (Take(std::move(s)) != 7) return 3;
  if (Which(d) != 1 || Which(std::move(d)) != 2 || Which(Derived{}) != 2)
    return 4;
  Base&& r = std::move(d);
  if (&r != static_cast<Base*>(&d)) return 5;
  Derived t(5);
  Derived a(std::move(t), std::allocator<int>());
  if (a.v != 105 || t.v != -2) return 6;
  return 0;
}
