// Dependent uses of a template type parameter inside a template body:
//   * `sizeof(R)` on the bare parameter must be measured per instantiation
//     (both when R is deduced and when it is supplied explicitly), and
//   * `R::member` (a qualified name through the parameter) must resolve to the
//     concrete type's static data member / static member function.

struct Big {
  char data[7];
};

struct ConfigA {
  static const int scale = 3;
  static int make() { return 9; }
};

template <class R>
int size_of() {
  return (int)sizeof(R);
}

template <class R>
int size_of_arg(R /*value*/) {
  return (int)sizeof(R);
}

template <class R>
int scale_of() {
  return R::scale;
}

template <class R>
int make_of() {
  return R::make();
}

template <class T>
struct WithNestedClass {
  class iterator {
   public:
    using value_type = T;
    value_type* p;
  };
};

template <class T>
struct WithDependentAlias {
  using mapped_type = T;

  int make(int key, const mapped_type& mapped) {
    return key + mapped.value;
  }

  int make(const mapped_type& mapped) {
    return mapped.value;
  }

  int call(int key) {
    return make(key, mapped_type());
  }
};

struct AliasValue {
  int value;
  AliasValue() : value(5) {}
};

int main() {
  if (size_of<Big>() != 7) {  // explicit type argument, used only in the body
    return 1;
  }
  Big b;
  if (size_of_arg(b) != 7) {  // deduced type argument
    return 2;
  }
  if (size_of<int>() != (int)sizeof(int)) {  // distinct instantiation
    return 3;
  }
  if (scale_of<ConfigA>() != 3) {  // dependent static data member
    return 4;
  }
  if (make_of<ConfigA>() != 9) {  // dependent static member function
    return 5;
  }
  if (scale_of<ConfigA>() * 2 != 6) {  // dependent value in a larger expression
    return 6;
  }
  WithNestedClass<int>::iterator it;
  int value = 11;
  it.p = &value;
  if (*it.p != 11) {
    return 7;
  }
  WithDependentAlias<AliasValue> alias;
  if (alias.call(4) != 9) {
    return 8;
  }
  return 0;
}
