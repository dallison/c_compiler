// RUN: -std=c++20

// Conversion operator templates parse in their various forms and are usable at
// a conversion site, where the template arguments are deduced from the target
// type.

struct Everything {
  int v;
  template <class T>
  operator T() const {
    return (T)v;
  }
};

struct WithPointer {
  int storage;
  template <class T>
  operator T*() {
    return (T*)&storage;
  }
};

struct ExplicitConv {
  int v;
  template <class T>
  explicit operator T() const {
    return (T)v;
  }
};

// A template conversion operator inherited from a base class.
struct Base {
  int v;
  template <class T>
  operator T() const {
    return (T)v;
  }
};
struct Derived : Base {};

void use() {
  Everything e{1};
  int i = e;
  long l = e;
  double d = e;
  (void)i;
  (void)l;
  (void)d;

  WithPointer p{2};
  int* ip = p;
  (void)ip;

  ExplicitConv ec{3};
  int j = static_cast<int>(ec);
  (void)j;

  Derived der;
  der.v = 4;
  int k = der;
  (void)k;
}
