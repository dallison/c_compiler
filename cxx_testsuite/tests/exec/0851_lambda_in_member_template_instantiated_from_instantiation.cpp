// `Outer<F>` instantiates `Inner<F>` from its own body, and Inner passes a
// lambda to a base-class member template.  The closure's call operator must be
// instantiated for `Inner<F>` (deducing its return type and emitting it), not
// left attached to the uninstantiated pattern (protobuf's
// ParseProto3Type::NewMsg -> NewDynamic -> WithDynamicType).
struct Base {
  template <typename F>
  static int With(int f, F body) {
    return body(1) + f;
  }
};

struct S : Base {
  template <typename F>
  static int Outer(int f, F body) {
    return Inner(f, body);
  }
  template <typename F>
  static int Inner(int f, F body) {
    return With(f, [&](int x) { return body(x) * 2; });
  }
  template <typename F>
  static int OuterTyped(int f, F body) {
    return InnerTyped(f, body);
  }
  template <typename F>
  static int InnerTyped(int f, F body) {
    return With(f, [&](int x) -> int { return body(x) + f; });
  }
};

int main() {
  volatile int five = 5;
  // body(1) = 10, doubled = 20, plus f = 25.
  if (S::Outer(five, [](int x) { return x * 10; }) != 25) {
    return 1;
  }
  // body(1) = 3, plus f = 8, plus f = 13.
  if (S::OuterTyped(five, [](int x) { return x + 2; }) != 13) {
    return 2;
  }
  return 0;
}
