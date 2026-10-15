// `Traits::Name(x)` with `Traits = Derived` names the static member Derived
// inherits from Base; the call must still bind to Base::Name, whose other
// callers (Base::Must) reference it under Base's mangled name (protobuf's json
// Proto3Type::TypeName).
struct Base {
  static int Name(int d) { return d; }
  static int Must(int d) { return Name(d) + 1; }
};
struct Derived : Base {};
struct MoreDerived : Derived {};

template <typename T>
struct TBase {
  static T Twice(T v) { return v * 2; }
  static T Quad(T v) { return Twice(Twice(v)); }
};
struct FromTemplate : TBase<int> {};

template <typename Traits>
int Use(int x) {
  return Traits::Must(x) + Traits::Name(x);
}

template <typename Traits>
int UseTemplateBase(int x) {
  return Traits::Quad(x) + Traits::Twice(x);
}

int main() {
  if (Use<Derived>(1) != 3) {
    return 1;
  }
  if (Use<MoreDerived>(2) != 5) {
    return 2;
  }
  if (Base::Must(4) != 5 || Base::Name(6) != 6) {
    return 3;
  }
  if (UseTemplateBase<FromTemplate>(1) != 6) {
    return 4;
  }
  return TBase<int>::Twice(5) == 10 ? 0 : 5;
}
