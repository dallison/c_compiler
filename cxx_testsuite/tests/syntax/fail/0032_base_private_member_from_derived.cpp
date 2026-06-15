// RUN: -std=c++17
// EXPECT: hidden is a private member of Base
struct Base {
 private:
  int hidden;
};

struct Derived : public Base {
  int read(void);
};

int Derived::read(void) {
  return hidden;
}
