// RUN: -std=c++17
// EXPECT_EXIT: 0

// A member of a private base at offset 0 made public by `using` is called on
// the derived object (the implicit object conversion ignores base access),
// and a polymorphic `typeid(*p)` as the left operand of `!=` keeps its value.
#include <typeinfo>

struct Counter {
  int count = 3;
  bool empty() const { return count == 0; }
  int size() const { return count; }
};
struct Middle : Counter {};
class Wrapper : private Middle {
 public:
  using Middle::empty;
  using Middle::size;
};

struct Shape {
  virtual ~Shape() {}
};
struct Circle : Shape {};

int main() {
  Wrapper w;
  if (w.empty() || w.size() != 3) return 1;
  const Wrapper& cw = w;
  if (cw.size() != 3) return 2;

  Circle circle;
  Shape* shape = &circle;
  if (typeid(*shape) != typeid(Circle)) return 3;
  if (!(typeid(*shape) != typeid(Shape))) return 4;
  return 0;
}
