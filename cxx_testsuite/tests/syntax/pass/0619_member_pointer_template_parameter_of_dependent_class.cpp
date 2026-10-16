// RUN: -std=c++20

// A non-type template parameter `int T::*` (or `void (T::*)()`) whose class
// is an earlier type parameter accepts `&Foo::n`, `nullptr` and a default,
// and a null argument stays null after substitution.
struct Foo {
  int n;
  void f() {}
};

template <class T, int T::*P>
struct Data {
  static constexpr bool null = P == nullptr;
  static_assert(P || null);
};
static_assert(!Data<Foo, &Foo::n>::null);
static_assert(Data<Foo, nullptr>::null);

template <class T, void (T::*P)()>
struct Function {
  static constexpr bool set = P;
};
static_assert(Function<Foo, &Foo::f>::set);
static_assert(!Function<Foo, nullptr>::set);

template <class T, int T::*P = nullptr>
struct Defaulted {
  static constexpr bool null = !P;
};
static_assert(Defaulted<Foo>::null);
static_assert(!Defaulted<Foo, &Foo::n>::null);

template <int Foo::*P = nullptr>
struct Concrete {
  static constexpr bool null = !P;
};
static_assert(Concrete<>::null);
static_assert(!Concrete<&Foo::n>::null);

template <class T, int T::*P>
int get(T& t) {
  return t.*P;
}

int main() {
  Foo foo{3};
  return get<Foo, &Foo::n>(foo) == 3 ? 0 : 1;
}
