// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Partial-construction cleanup across a virtual base: when a constructor throws
// after building its virtual base (and other subobjects), the complete object
// destroys them in reverse construction order, and the virtual base is
// destroyed exactly once -- by the complete object, never by a base-subobject
// constructor variant. The incomplete object's own destructor never runs.
// Results are reported through the process exit code.

int g_seq[64];
int g_n = 0;

static void reset() { g_n = 0; }

struct Tracer {
  int id;
  explicit Tracer(int i) : id(i) {}
  ~Tracer() { g_seq[g_n++] = id; }
};

// Virtual base with its own member and a destructor body.
struct V {
  Tracer vm;
  V() : vm(1) {}
  ~V() { g_seq[g_n++] = 2; }  // body, then vm(1)
};

// Throws right after its virtual base and member are built.
struct DThrow : virtual V {
  Tracer dm;
  DThrow() : V(), dm(3) { throw 1; }
  ~DThrow() { g_seq[g_n++] = 999; }  // must never run
};

// Fully constructible; used as a base of the most-derived class so it runs as a
// base-subobject constructor variant (which must NOT build or destroy V).
struct DBase : virtual V {
  Tracer dm;
  DBase() : V(), dm(3) {}
  ~DBase() { g_seq[g_n++] = 8; }  // body, then dm(3)
};

// Most-derived class: it (not DBase) builds the shared virtual base V, then
// throws after its own member is built.
struct D2 : DBase {
  Tracer d2m;
  D2() : DBase(), d2m(5) { throw 1; }
  ~D2() { g_seq[g_n++] = 999; }  // must never run
};

int main() {
  // 1) Direct virtual base: reverse order dm(3), then V (body 2, then vm 1);
  //    ~DThrow never runs.
  reset();
  try {
    DThrow d;
    (void)d;
  } catch (...) {
  }
  if (g_n != 3) return 100 + g_n;
  if (g_seq[0] != 3) return 10;
  if (g_seq[1] != 2) return 11;
  if (g_seq[2] != 1) return 12;

  // 2) Virtual base shared through a base subobject: the complete object D2
  //    builds V, DBase is built as a base subobject (does not touch V). A throw
  //    in D2's body destroys d2m(5), then ~DBase (body 8, then dm 3), then the
  //    virtual base V exactly once (body 2, then vm 1). ~D2 never runs.
  reset();
  try {
    D2 d;
    (void)d;
  } catch (...) {
  }
  if (g_n != 5) return 120 + g_n;
  if (g_seq[0] != 5) return 20;
  if (g_seq[1] != 8) return 21;
  if (g_seq[2] != 3) return 22;
  if (g_seq[3] != 2) return 23;
  if (g_seq[4] != 1) return 24;

  // 3) Normal (non-throwing) path is unaffected: V built once, destroyed once.
  reset();
  {
    DBase b;
    (void)b;
  }
  if (g_n != 4) return 140 + g_n;
  if (g_seq[0] != 8) return 30;  // ~DBase body
  if (g_seq[1] != 3) return 31;  // dm
  if (g_seq[2] != 2) return 32;  // ~V body
  if (g_seq[3] != 1) return 33;  // vm

  return 0;
}
