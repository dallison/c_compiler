// A class template that names its own current instantiation with an explicit
// template-id (`Box<T>`) inside its own members.  While a class template's body
// is parsed the class is not yet marked as a template, so both the declaration
// form (`Box<T> tmp;`) and the functional-cast expression form (`Box<T>()`)
// previously failed to parse.  Both should behave like the bare
// injected-class-name `Box`.

template <class T>
struct Box {
  T value;
  Box() { value = T(); }
  explicit Box(T v) { value = v; }
  T get() { return value; }

  // Functional-cast expression form: default-constructed self temporary.
  T sum_default() { return Box<T>().get() + get(); }

  // Functional-cast expression form with an argument.
  T via_arg(T x) { return Box<T>(x).get(); }

  // Declaration form: a local of the current instantiation.
  T via_decl(T x) {
    Box<T> tmp;
    tmp.value = x;
    return tmp.get();
  }

  // Nested: a self temporary feeding another self temporary.
  T nested(T x) { return Box<T>(Box<T>(x).get()).get(); }
};

// A different class template with a name that shares a prefix, to confirm the
// self-reference match is exact (does not over-match on prefixes).
template <class T>
struct BoxHolder {
  Box<T> b;
  T unwrap() { return b.get(); }
};

int main() {
  Box<int> b;
  b.value = 10;

  if (b.sum_default() != 10) {  // 0 + 10
    return 1;
  }
  if (b.via_arg(7) != 7) {
    return 2;
  }
  if (b.via_decl(9) != 9) {
    return 3;
  }
  if (b.nested(4) != 4) {
    return 4;
  }

  BoxHolder<int> h;
  h.b.value = 5;
  if (h.unwrap() != 5) {
    return 5;
  }

  // A local named like the class followed by '<' must remain a less-than
  // comparison, not a template-id.
  int Box = 3;
  if (!(Box < 5)) {
    return 6;
  }

  return 0;
}
