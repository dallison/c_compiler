// RUN: -std=c++20
// EXPECT: explicit specifier must be a constant expression
int runtime_value(void) {
  return 1;
}

struct ExplicitByRuntime {
  explicit(runtime_value()) operator int() const {
    return 1;
  }
};

int main(void) {
  ExplicitByRuntime value;
  return static_cast<int>(value);
}
