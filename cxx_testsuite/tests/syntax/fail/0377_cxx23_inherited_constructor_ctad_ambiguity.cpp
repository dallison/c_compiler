// RUN: -std=c++23
// EXPECT: Ambiguous class template argument deduction for AmbiguousDerived

template <class T>
struct FirstBase {
  FirstBase(int) {
  }
};

FirstBase(int) -> FirstBase<int>;

template <class T>
struct SecondBase {
  SecondBase(int) {
  }
};

SecondBase(int) -> SecondBase<long*>;

template <class T>
struct AmbiguousDerived : FirstBase<T>, SecondBase<T*> {
  using FirstBase<T>::FirstBase;
  using SecondBase<T*>::SecondBase;
};

AmbiguousDerived value(1);
