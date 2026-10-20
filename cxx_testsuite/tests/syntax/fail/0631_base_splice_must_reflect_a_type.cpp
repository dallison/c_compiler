// RUN: -std=c++26
// EXPECT: Type splice operand does not reflect a type

#include <meta>

int f();

struct X : [:^^f:] {
  int a;
  int get() { return a; }
};

int main() {
  X x;
  return x.get();
}
