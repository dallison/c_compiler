// Direct-initialization considers explicit conversion functions yielding the
// target type ([over.match.conv]): `bool b(lease)`, `bool b{lease}` and the
// mem-initializer `success_(value)` in gtest's AssertionResult.
struct Lease {
  bool held;
  explicit operator bool() const { return held; }
};

struct Count {
  int n;
  explicit operator int() const { return n; }
};

struct Result {
  template <typename T>
  explicit Result(const T& value) : success_(value) {}
  bool success_;
};

template <typename T>
static bool DirectInTemplate(const T& value) {
  bool b(value);
  return b;
}

int main() {
  Lease held{true};
  Lease free{false};
  bool a(held);
  bool b{free};
  if (!a || b) return 1;
  Count seven{7};
  int n(seven);
  if (n != 7) return 2;
  if (!Result(held).success_ || Result(free).success_) return 3;
  if (!DirectInTemplate(held) || DirectInTemplate(free)) return 4;
  if (!held || free) return 5;
  return 0;
}
