// RUN: -std=c++20
// EXPECT: 0602_error_then_scope_exit_destructors.cpp:9: No such symbol "undeclared"

// An error in one function must not disable scope-exit destructor insertion in
// the rest of the translation unit: the constant evaluations below would then
// leak their allocations and fail with spurious diagnostics.

int Broken() {
  return undeclared;
}

struct Owner {
  int* p;
  constexpr explicit Owner(int v) : p(new int(v)) {}
  constexpr ~Owner() { delete p; }
};

constexpr int EarlyReturn(bool b) {
  Owner o(3);
  if (b) {
    return *o.p;
  }
  return 0;
}
static_assert(EarlyReturn(true) == 3);

constexpr int LoopBreak() {
  int sum = 0;
  for (int i = 0; i < 4; i++) {
    Owner o(i);
    if (i == 2) {
      break;
    }
    sum += *o.p;
  }
  return sum;
}
static_assert(LoopBreak() == 1);

int main() { return 0; }
