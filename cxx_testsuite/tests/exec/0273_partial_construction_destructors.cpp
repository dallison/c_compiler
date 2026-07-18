// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Partial-construction cleanup: when a constructor throws after constructing
// some of its base/member subobjects, exactly those already-constructed
// subobjects are destroyed in reverse construction order, and the incomplete
// object's own destructor does NOT run.  Fully-constructed objects are
// unaffected (destroyed exactly once, never via a cleanup pad).  Results are
// reported through the process exit code.

int g_seq[64];
int g_n = 0;

static void reset() { g_n = 0; }

struct Tracer {
  int id;
  explicit Tracer(int i) : id(i) {}
  ~Tracer() { g_seq[g_n++] = id; }
};

// A class whose constructor always throws after building both members.
struct Members {
  Tracer a;
  Tracer b;
  Members() : a(1), b(2) { throw 1; }
  ~Members() { g_seq[g_n++] = 999; }  // must never run
};

// A fully-constructible base with its own member and destructor body.
struct Base {
  Tracer base_member;
  Base() : base_member(10) {}
  ~Base() { g_seq[g_n++] = 20; }  // body, then base_member(10)
};

// A derived class that throws after its base and member are built.
struct Der : Base {
  Tracer m;
  Der() : Base(), m(30) { throw 1; }
  ~Der() { g_seq[g_n++] = 999; }  // must never run
};

// A non-throwing aggregate to exercise the normal (fall-through) path.
struct Good {
  Tracer a;
  Tracer b;
  Good() : a(1), b(2) {}
  ~Good() { g_seq[g_n++] = 7; }  // body, then b(2), then a(1)
};

int main() {
  // 1) Member subobjects: b then a; ~Members never runs.
  reset();
  try {
    Members m;
    (void)m;
  } catch (...) {
  }
  if (g_n != 2) return 100 + g_n;
  if (g_seq[0] != 2) return 10;
  if (g_seq[1] != 1) return 11;

  // 2) Base + member subobjects: m(30), then ~Base (body 20, then member 10);
  //    ~Der never runs.
  reset();
  try {
    Der d;
    (void)d;
  } catch (...) {
  }
  if (g_n != 3) return 120 + g_n;
  if (g_seq[0] != 30) return 20;
  if (g_seq[1] != 20) return 21;
  if (g_seq[2] != 10) return 22;

  // 3) A live local before the throwing object: subobjects first, then local.
  reset();
  try {
    Tracer local(70);
    Members m;
    (void)m;
    (void)local;
  } catch (...) {
  }
  if (g_n != 3) return 140 + g_n;
  if (g_seq[0] != 2) return 30;   // Members::b
  if (g_seq[1] != 1) return 31;   // Members::a
  if (g_seq[2] != 70) return 32;  // local

  // 4) Normal path: full construction destroys once, in order, via ~Good.
  reset();
  {
    Good g;
    (void)g;
  }
  if (g_n != 3) return 160 + g_n;
  if (g_seq[0] != 7) return 40;  // ~Good body
  if (g_seq[1] != 2) return 41;  // b
  if (g_seq[2] != 1) return 42;  // a

  return 0;
}
