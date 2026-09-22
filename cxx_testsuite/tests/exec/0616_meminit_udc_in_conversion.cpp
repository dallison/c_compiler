// RUN: -std=c++17
// EXPECT_EXIT: 0

// Implicit conversion into a class-template constructor must still allow that
// constructor's mem-initializer its own user-defined conversion.  Abseil's
// `FormatSpecTemplate(const char* s) : Base(s)` needs `const char*` to bind
// to `UntypedFormatSpec(string_view)`.

struct View {
  const char* p;
  View(const char* s) : p(s) {}
};

struct Base {
  Base() = delete;
  Base(const Base&) = delete;
  explicit Base(View v) : n(v.p != 0 ? 3 : 0) {}
  int n;
};

template <int N>
struct Spec : Base {
  using B = Base;
  Spec(const char* s) : B(s) {}
};

template <typename T>
int take(const Spec<1>& s) {
  (void)sizeof(T);
  return s.n;
}

int main() { return take<int>("abcd") == 3 ? 0 : 1; }
