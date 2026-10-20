// RUN: -std=c++20
// EXPECT: No matching overload for R

struct R {
  R(int) {}
};

struct H {
  R r;
  H() {}
};

int main() {
  H h;
  (void)h;
  return 0;
}
