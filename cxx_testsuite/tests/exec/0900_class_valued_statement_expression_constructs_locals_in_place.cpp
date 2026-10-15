// A local constructed inside a class-valued GNU statement expression gets its
// own storage; only the final expression initializes the declared object.
static int live = 0;

struct Lease {
  long a = 0, b = 0, c = 0, d = 0;
};

struct Holder {
  Lease value{7, 8, 9, 10};
  bool ok = true;
};

struct Check {
  explicit Check(bool ok) : ok(ok) { live++; }
  Check(const Check& other) : ok(other.ok) { live++; }
  ~Check() { live--; }
  explicit operator bool() const { return ok; }
  bool ok;
};

struct Tracked {
  explicit Tracked(long v) : v(v) { live++; }
  Tracked(const Tracked& other) : v(other.v) { live++; }
  Tracked(Tracked&& other) noexcept : v(other.v) {
    other.v = -1;
    live++;
  }
  ~Tracked() { live--; }
  long v;
};

int Plain() {
  Lease got = ({
    Holder h;
    h.value;
  });
  return got.a == 7 && got.d == 10 ? 0 : 1;
}

int WithCondition() {
  Lease got = ({
    Holder h;
    if (const Check check = Check(h.ok))
      ;
    else
      return 2;
    h.value;
  });
  return got.b == 8 && live == 0 ? 0 : 3;
}

int NonTrivialResult() {
  {
    Tracked got = ({
      Holder h;
      Tracked t(h.value.c);
      t;
    });
    if (got.v != 9 || live != 1) return 4;
  }
  return live == 0 ? 0 : 5;
}

int main() {
  if (int rc = Plain()) return rc;
  if (int rc = WithCondition()) return rc;
  if (int rc = NonTrivialResult()) return rc;
  return 0;
}
