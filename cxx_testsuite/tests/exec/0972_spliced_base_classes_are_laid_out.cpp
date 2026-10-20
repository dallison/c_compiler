// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <meta>

template <class T>
struct Box {
  T value;
};

struct Shape {
  virtual ~Shape() {}
  virtual int Sides() const { return 0; }
  int id = 7;
};

struct Direct : [:^^Box<int>:] {
  int extra;
};

constexpr auto kLongBox = ^^Box<long>;
struct ViaVariable : [:kLongBox:] {
  int extra;
};

struct Tag {
  char tag;
};
struct Mixed : Tag, [:^^Box<short>:] {
  short extra;
};

struct Square : [:^^Shape:] {
  int Sides() const override { return 4; }
};

template <std::meta::info R>
struct FromTemplate : [:R:] {
  int extra = 9;
};

int SidesOf(const Shape& shape) { return shape.Sides(); }

int main() {
  static_assert(sizeof(Direct) == 2 * sizeof(int));
  static_assert(sizeof(ViaVariable) == 2 * sizeof(long));
  static_assert(sizeof(Mixed) == sizeof(Tag) + 1 + 2 * sizeof(short));
  static_assert(sizeof(FromTemplate<^^Box<int>>) == 2 * sizeof(int));

  Direct d;
  d.value = 3;
  d.extra = 4;
  if (d.value + d.extra != 7) return 1;
  Box<int>& as_base = d;
  if (as_base.value != 3) return 2;

  ViaVariable v;
  v.value = 5;
  v.extra = 6;
  if (v.value + v.extra != 11) return 3;

  Mixed m;
  m.tag = 'x';
  m.value = 12;
  m.extra = 13;
  Box<short>* box = &m;
  if (box->value != 12 || m.tag != 'x' || m.extra != 13) return 4;

  Square s;
  if (SidesOf(s) != 4 || s.id != 7) return 5;

  FromTemplate<^^Box<int>> t;
  t.value = 1;
  if (t.value + t.extra != 10) return 6;
  return 0;
}
