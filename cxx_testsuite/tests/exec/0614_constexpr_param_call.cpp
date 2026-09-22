// RUN: -std=c++17
// EXPECT_EXIT: 0

// Abseil FormatSpecTemplate::CheckArity uses defaulted class-type parameters
// so `static_assert(SpecifierCount(i) == ParametersPassed(j))` names the
// operands in diagnostics.  Instantiating the constexpr function must fold
// those member calls even though the parameters were never bound by a call.

template <bool res>
struct ErrorMaker {
  constexpr bool operator()(int) const { return res; }
};

template <int i, int j>
static constexpr bool CheckArity(ErrorMaker<true> SpecifierCount = {},
                                 ErrorMaker<i == j> ParametersPassed = {}) {
  static_assert(SpecifierCount(i) == ParametersPassed(j), "arity");
  return SpecifierCount(i) == ParametersPassed(j);
}

template <int X>
struct Outer {
  template <bool res>
  struct ErrorMaker {
    constexpr bool operator()(int) const { return res; }
  };
  template <int i, int j>
  static constexpr bool Check(ErrorMaker<true> SpecifierCount = {},
                              ErrorMaker<i == j> ParametersPassed = {}) {
    static_assert(SpecifierCount(i) == ParametersPassed(j), "nested");
    return true;
  }
};

int main() {
  return CheckArity<1, 1>() && CheckArity<3, 3>() && Outer<1>::Check<1, 1>()
             ? 0
             : 1;
}
