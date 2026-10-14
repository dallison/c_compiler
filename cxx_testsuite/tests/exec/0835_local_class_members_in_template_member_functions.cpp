// Member functions of a local class are defined when the enclosing function is
// instantiated, whether that function is a function template, a member of a
// class template instantiated on demand, or a member function template of an
// ordinary class (absl::Flag<T>::Get, protobuf Reflection::SwapOneofField).
template <typename T>
T FreeGet(T v) {
  struct S {
    T value;
    S(T x) : value(x) {}
    ~S() {}
    int Twice() const { return 2; }
  };
  S s(v);
  return s.value * s.Twice();
}

template <typename T>
struct Box {
  T v;
  T Get() const {
    struct S {
      T value;
      S(T x) : value(x) {}
      ~S() {}
    };
    S s(v);
    return s.value;
  }
};

struct Reflection {
  template <bool kDouble>
  int Swap(int x) const {
    struct Wrapper {
      int v;
      Wrapper(int a) : v(a) {}
      int Get() const { return kDouble ? v * 2 : v; }
    };
    Wrapper w(x);
    return w.Get();
  }
};

int main() {
  if (FreeGet(3) != 6) return 1;
  Box<int> b{5};
  if (b.Get() != 5) return 2;
  Reflection r;
  if (r.Swap<true>(3) != 6) return 3;
  if (r.Swap<false>(4) != 4) return 4;
  return 0;
}
