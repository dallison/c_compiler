// A class template-id used as a functional cast, e.g. `A<int>()`, must
// instantiate the named specialization directly rather than attempting class
// template argument deduction from the (possibly empty) constructor arguments.
// This exercises the temporary in value contexts (member call, initializer,
// argument) at both concrete and dependent (template-body) scope.

template <class T>
struct Wrap {
  T value;
  Wrap() { value = T(); }
  explicit Wrap(T v) { value = v; }
  T get() { return value; }
};

// A distinct template so the dependent case builds a temporary of a *different*
// template than the enclosing one (the injected-class-name is a separate path).
template <class T>
struct User {
  T total;
  User() { total = Wrap<T>().get() + Wrap<T>(T() + 5).get(); }
};

template <class T>
T sum_via_temporary(T x) {
  // Member call on a temporary specialization in a dependent context.
  return Wrap<T>().get() + Wrap<T>(x).get();
}

int main() {
  // Concrete: member call on a temporary.
  if (Wrap<int>().get() != 0) {
    return 1;
  }
  if (Wrap<int>(7).get() != 7) {
    return 2;
  }

  // Concrete: temporary as an initializer (copy-init).
  Wrap<int> w = Wrap<int>(9);
  if (w.get() != 9) {
    return 3;
  }

  // Dependent: temporary member call inside a function template.
  if (sum_via_temporary<int>(4) != 4) {
    return 4;
  }

  // Dependent: temporary member call inside a class template member.
  User<int> u;
  if (u.total != 5) {
    return 5;
  }

  return 0;
}
