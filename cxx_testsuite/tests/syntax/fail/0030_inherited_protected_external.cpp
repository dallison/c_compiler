// RUN: -std=c++17
// EXPECT: protected_value is a protected member of Base
struct Base {
 protected:
  int protected_value;
};

struct Derived : public Base {
};

int main(void) {
  Derived derived;
  return derived.protected_value;
}
