// RUN: -std=c++20
//
// A thrown derived object is caught by a handler naming a public base class.

struct Base {
  int value;
};

struct Derived : Base {
  int extra;
};

int main(void) {
  try {
    Derived d;
    d.value = 42;
    d.extra = 7;
    throw d;
    return 1;
  } catch (Base& b) {
    return b.value - 42;  // 0 on success
  } catch (...) {
    return 2;
  }
}
