// RUN: -std=c++20
// EXPECT: Use of implicitly deleted function B

struct R {
  R(int) {}
};

struct B : R {};

int main() {
  B b;
  (void)b;
  return 0;
}
