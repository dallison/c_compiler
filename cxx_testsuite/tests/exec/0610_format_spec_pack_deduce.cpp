// RUN: -std=c++17
// EXPECT_EXIT: 0

// StreamFormat-shaped deduction: the first parameter is an alias of a
// class-template-id that mentions Args, but the actual argument is a string
// literal (convertible via constructor).  Args must be deduced from the
// later function-parameter pack.

template <int... Xs>
struct SpecT {
  SpecT(const char*) {}
};

template <typename T>
constexpr int Conv() {
  return 1;
}

template <typename... Args>
using Spec = SpecT<Conv<Args>()...>;

template <typename... Args>
int Format(const Spec<Args...>&, const Args&... args) {
  return (int)sizeof...(args);
}

int main() {
  return Format("%08x", 1u) == 1 ? 0 : 1;
}
