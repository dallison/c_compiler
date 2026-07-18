// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// RAII during exception propagation: an exception thrown in a nested call must
// destroy the automatic objects of every frame it unwinds through, in reverse
// construction order -- including frames that have no try/catch of their own.
// Results are reported through the process exit code because the exec harness
// validates exit status, not stdout.

int g_seq[64];
int g_n = 0;

struct T {
  int id;
  explicit T(int i) : id(i) {}
  ~T() { g_seq[g_n++] = id; }
};

static void deepest() {
  T a(1);
  T b(2);
  throw 42;  // unwinding this frame must destroy b then a
  T unreached(99);
  (void)unreached;
}

static void middle() {
  T c(3);  // destroyed while unwinding through middle() (no try here)
  deepest();
}

static void outer() {
  T d(4);
  middle();
}

int main() {
  bool caught = false;
  try {
    outer();
  } catch (int e) {
    if (e != 42) return 60;
    caught = true;
  }
  if (!caught) return 61;
  // Reverse construction order across every unwound frame: b, a, c, d.
  if (g_n != 4) return 100 + g_n;
  if (g_seq[0] != 2) return 10;
  if (g_seq[1] != 1) return 11;
  if (g_seq[2] != 3) return 12;
  if (g_seq[3] != 4) return 13;
  return 0;
}
