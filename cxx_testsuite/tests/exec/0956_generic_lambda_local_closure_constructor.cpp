// RUN: -std=c++17
// A class template specialized on a closure type local to a generic lambda is
// constructed inside that lambda.  Each instantiation of the lambda has its
// own closure type, so the constructor call must bind to the specialization
// for the instantiation's closure, not the one from the lambda's pattern.
template <typename T> struct W {
  T t;
  explicit W(T x) : t(x) {}
  int operator()(int v) const { return t(v); }
};
template <typename T> W(T) -> W<T>;

template <typename... Fs> struct Overload : Fs... {
  using Fs::operator()...;
};
template <typename... Fs> Overload(Fs...) -> Overload<Fs...>;

int main() {
  auto explicit_args = [](auto x) {
    auto l = [](int p) { return p + 1; };
    const auto w = W<decltype(l)>{l};
    return w(x);
  };
  if (explicit_args(4) != 5) return 1;

  auto deduced = [](auto x) {
    const auto w = W{[](int p) { return p + 1; }};
    return w(x);
  };
  if (deduced(4) != 5) return 2;

  auto overloaded = [](auto x) {
    auto o = Overload{[](int p) { return p + 10; },
                      [](const char*) { return 0; }};
    return o(x);
  };
  if (overloaded(4) != 14) return 3;
  return 0;
}
