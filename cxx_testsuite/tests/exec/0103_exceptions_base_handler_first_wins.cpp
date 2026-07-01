// RUN: -std=c++20
//
// When several handlers of one try can match, the first one in source order is
// chosen ([except.handle]).  A base-class handler listed before the exact-type
// handler must therefore win.

struct Base {
  int value;
};

struct Derived : Base {
  int extra;
};

int main(void) {
  try {
    Derived d;
    d.value = 10;
    d.extra = 20;
    throw d;
    return 1;
  } catch (Base& b) {
    return b.value - 10;  // 0: base handler selected first
  } catch (Derived& d) {
    return 2;  // must not be reached
  }
}
