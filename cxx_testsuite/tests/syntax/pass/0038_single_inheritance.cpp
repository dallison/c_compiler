// RUN: -std=c++17
struct Base {
  int value;
  int get(void);
};

int Base::get(void) {
  return value;
}

struct Derived : public Base {
  int extra;
};

int main(void) {
  Derived derived;
  derived.value = 3;
  derived.extra = 4;
  return derived.get() + derived.extra;
}
