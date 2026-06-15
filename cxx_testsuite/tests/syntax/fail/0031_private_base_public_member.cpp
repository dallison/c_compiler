// RUN: -std=c++17
// EXPECT: value is a private member of Base
struct Base {
 public:
  int value;
};

struct Derived : private Base {
};

int main(void) {
  Derived derived;
  return derived.value;
}
