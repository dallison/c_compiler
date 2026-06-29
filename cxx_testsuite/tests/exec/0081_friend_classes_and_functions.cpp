// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Friend classes and friend functions may access the private and protected
// members of the class that grants them friendship.

struct Vec {
 private:
  int x;
  int y;

 public:
  Vec(int a, int b) : x(a), y(b) {}

  // Friend class: every member of Adder may touch Vec's privates.
  friend class Adder;
  // Friend function declared here, defined later at namespace scope.
  friend int dot(const Vec& a, const Vec& b);
  // Inline friend operator: the canonical use of friend functions.
  friend Vec operator+(const Vec& a, const Vec& b) {
    return Vec(a.x + b.x, a.y + b.y);
  }
};

struct Adder {
  int sum(const Vec& v) const { return v.x + v.y; }
};

int dot(const Vec& a, const Vec& b) { return a.x * b.x + a.y * b.y; }

int main(void) {
  Vec a(1, 2);
  Vec b(3, 4);

  Adder adder;
  if (adder.sum(a) != 3) {
    return 1;
  }
  if (dot(a, b) != 1 * 3 + 2 * 4) {
    return 2;
  }
  Vec c = a + b;
  Adder check;
  if (check.sum(c) != (1 + 3) + (2 + 4)) {
    return 3;
  }
  return 0;
}
