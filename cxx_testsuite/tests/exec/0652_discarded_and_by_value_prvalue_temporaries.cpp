// RUN: -std=c++20
// EXPECT_EXIT: 0

// A class prvalue returned by a call creates an object even when its value is
// discarded or passed straight to a by-value parameter.  It is destroyed at
// the end of the full-expression, including when the full-expression throws.
static int alive = 0;
static int constructed = 0;

struct P {
  int v = 1;
  P() { ++alive; ++constructed; }
  P(const P& o) : v(o.v) { ++alive; ++constructed; }
  ~P() { --alive; }
  int f() const { return v; }
};

P make() { return P(); }
void sink(P) {}
void sink_throw(P) { throw 7; }
int sink2(P, P) { return 0; }
struct Q {
  Q(P) {}
};
struct QThrow {
  QThrow(P) { throw 8; }
};

#define CHECK(n, stmt)          \
  do {                          \
    alive = 0;                  \
    stmt;                       \
    if (alive != 0) return n;   \
  } while (0)

#define CHECK_THROWS(n, stmt)   \
  do {                          \
    alive = 0;                  \
    try {                       \
      stmt;                     \
      return 100 + n;           \
    } catch (int) {             \
    }                           \
    if (alive != 0) return n;   \
  } while (0)

int main() {
  CHECK(1, make());
  CHECK(2, (void)make());
  CHECK(3, make().f());
  CHECK(4, sink(make()));
  CHECK(5, sink2(make(), P()));
  CHECK(6, { Q q(make()); });
  CHECK(7, { Q q{make()}; });
  CHECK(8, (make(), 0));
  CHECK(9, (alive >= 0 ? make() : P()));
  CHECK(10, sink(alive >= 0 ? make() : P()));
  CHECK(11, for (make(); constructed < 0; make()) {});

  CHECK_THROWS(20, sink_throw(make()));
  CHECK_THROWS(21, sink_throw(P()));
  CHECK_THROWS(22, sink_throw(alive >= 0 ? make() : P()));
  CHECK_THROWS(23, { QThrow q(make()); });
  CHECK_THROWS(24, (sink(make()), sink_throw(make())));

  constructed = 0;
  sink(make());
  if (constructed != 1) return 30;
  return 0;
}
