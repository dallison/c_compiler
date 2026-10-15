// Inside class template `W<T>`, `W<char>`, `W<T*>` and `W<U>` name other
// specializations; only `W<T>` (or bare `W`) is the current instantiation.
// Their members must not resolve to the class being defined.  Compile-time
// recursion through the injected-class-name (googletest's
// TypeParameterizedTest<..., typename Types::Tail>::Register) relies on this.
template <class T> struct W {
  typedef T type;
  static const int kSize = sizeof(T);
  static int Size() { return sizeof(T); }
  static int Concrete() { return W<char>::Size(); }
  static int ConcreteType() { return sizeof(typename W<char>::type); }
  static int ConcreteTypeNoTypename() {
    W<char>::type c = 0;
    return sizeof(c);
  }
  static int ConcreteValue() { return W<char>::kSize; }
  static int ViaTypedef() {
    typedef W<char> WC;
    return WC::Size();
  }
  static int ViaPointer() {
    int (*f)() = &W<char>::Size;
    return f();
  }
  static int Global() { return ::W<short>::Size(); }
  template <class U> static int Member() { return W<U>::Size(); }
  static int Dependent() { return W<T*>::Size(); }
  static int DependentType() { return sizeof(typename W<T*>::type); }
  static int Self() { return W<T>::Size(); }
  static int Bare() { return W::Size(); }
};

struct None {};
template <class H, class T> struct Cons {
  typedef H Head;
  typedef T Tail;
};
template <class Types> struct Count {
  static int Run(int n) { return Count<typename Types::Tail>::Run(n + 1); }
};
template <> struct Count<None> {
  static int Run(int n) { return n; }
};

template <template <class> class Fixture, class Types> struct Register {
  static int Run(int n) {
    return Register<Fixture, typename Types::Tail>::Run(n + sizeof(Fixture<typename Types::Head>));
  }
};
template <template <class> class Fixture> struct Register<Fixture, None> {
  static int Run(int n) { return n; }
};
template <class T> struct Box {
  T value;
};

// Inside a partial specialization, `Get<I - 1, ...>` and `P<T>` are chosen
// among the primary's specializations, not the partial being defined.
template <int I, class List> struct Get;
template <class H, class T> struct Get<0, Cons<H, T>> {
  static int Size() { return sizeof(H); }
};
template <int I, class H, class T> struct Get<I, Cons<H, T>> {
  static int Size() { return Get<I - 1, T>::Size(); }
  static int TailSize() { return sizeof(typename Get<I - 1, T>::Tag); }
  struct Tag {
    char c[I];
  };
};
template <class T> struct P {
  static int Depth() { return 0; }
};
template <class T> struct P<T*> {
  static int Depth() { return 1 + P<T>::Depth(); }
};

int main() {
  if (W<double>::Concrete() != 1) return 1;
  if (W<double>::ConcreteType() != 1) return 2;
  if (W<double>::ConcreteTypeNoTypename() != 1) return 3;
  if (W<double>::ConcreteValue() != 1) return 4;
  if (W<double>::ViaTypedef() != 1) return 5;
  if (W<double>::ViaPointer() != 1) return 6;
  if (W<double>::Global() != 2) return 7;
  if (W<double>::Member<short>() != 2) return 8;
  if (W<char>::Dependent() != sizeof(void*)) return 9;
  if (W<char>::DependentType() != sizeof(void*)) return 10;
  if (W<short>::Self() != 2) return 11;
  if (W<short>::Bare() != 2) return 12;
  if (Count<Cons<int, Cons<double, Cons<char, None>>>>::Run(0) != 3) return 13;
  if (Register<Box, Cons<char, Cons<short, None>>>::Run(0) != 3) return 14;
  typedef Cons<int, Cons<short, Cons<char, None>>> List;
  if (Get<2, List>::Size() != 1) return 15;
  if (Get<1, List>::Size() != 2) return 16;
  if (Get<2, List>::TailSize() != 1) return 17;
  if (P<int***>::Depth() != 3) return 18;
  return 0;
}
