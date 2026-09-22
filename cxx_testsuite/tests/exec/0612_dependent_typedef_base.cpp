// RUN: -std=c++17
// EXPECT_EXIT: 0

// Abseil FormatSpecTemplate inherits
// `MakeDependent<UntypedFormatSpec, Args...>::type`.  A concrete
// `Owner<Args>::type` nested typedef must become the real base class.

template <typename T, int...>
struct MakeDependent {
  using type = T;
};

struct Base {
  int spec_;
  explicit Base(const char*) : spec_(3) {}
};

struct Direct : public MakeDependent<Base, 7>::type {
  Direct(const char* s) : Base(s) {}
};

template <int... Xs>
struct Spec : public MakeDependent<Base, Xs...>::type {
  using B = typename MakeDependent<Base, Xs...>::type;
  Spec(const char* s) : B(s) {}
};

int main() {
  Direct d("x");
  Spec<7> s("y");
  return (d.spec_ == 3 && s.spec_ == 3) ? 0 : 1;
}
