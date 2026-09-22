// RUN: -std=c++17
// EXPECT_EXIT: 0

// FormatSpec<Args...> is FormatSpecTemplate<Conv<Args>()...>.  Deduce Args
// from later parameters, then construct the alias from a string literal.

template <typename T>
constexpr int Conv() {
  return 1;
}

struct US {
  US() = delete;
  US(const US&) = delete;
  explicit US(const char*) {}
  int n = 3;
};

template <int... Xs>
struct SpecT : US {
  SpecT(const char* s) : US(s) {}
};

template <typename... Args>
using Spec = SpecT<Conv<Args>()...>;

template <typename... Args>
int wrap(const Spec<Args...>& format, const Args&... args) {
  (void)format;
  return (int)sizeof...(args);
}

int main() {
  Spec<unsigned> direct("%08x");
  return (direct.n == 3 && wrap("%08x", 1u) == 1) ? 0 : 1;
}
