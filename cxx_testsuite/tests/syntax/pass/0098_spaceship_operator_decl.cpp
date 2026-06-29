// RUN: -std=c++20
// Declaration and definition of operator<=> in member and free forms parse and
// type-check (return type is a plain int here so the test needs no <compare>).
struct Box {
  int value;
  int operator<=>(const Box& other) const;
};

int Box::operator<=>(const Box& other) const {
  return value - other.value;
}

struct Point {
  int n;
};

int operator<=>(const Point& a, const Point& b) {
  return a.n - b.n;
}

int use(const Box& a, const Box& b, const Point& p, const Point& q) {
  return (a <=> b) + (p <=> q);
}
