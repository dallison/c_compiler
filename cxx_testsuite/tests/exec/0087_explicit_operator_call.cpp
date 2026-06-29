// Explicit (named) operator and conversion calls through member access:
//   x.operator+(y), x.operator()(y), x.operator[](i), p->operator()(y),
//   x.operator int(), x.operator Box(), x.operator int*()
// These name the operator/conversion function directly instead of using the
// operator syntax, and must resolve to the same member function.

struct Adder {
  int base;
  int operator()(int x) const { return base + x; }
  int operator+(int x) const { return base + x; }
  int operator[](int i) const { return base + i; }
  bool operator<(const Adder& o) const { return base < o.base; }
};

struct Box {
  int v;
};

struct Convertible {
  int v;
  operator int() const { return v; }
  operator Box() const { return Box{v + 1}; }
};

struct Holder {
  int storage;
  operator int*() { return &storage; }
};

int main() {
  Adder a{10};

  // operator-function-ids.
  if (a.operator()(5) != 15) return 1;
  if (a.operator+(7) != 17) return 2;
  if (a.operator[](3) != 13) return 3;

  // Through a pointer (operator-> / arrow access).
  Adder* p = &a;
  if (p->operator()(8) != 18) return 4;

  // operator< named explicitly.
  Adder b{20};
  if (!a.operator<(b)) return 5;

  // conversion-function-ids.
  Convertible c{42};
  if (c.operator int() != 42) return 6;
  if (c.operator Box().v != 43) return 7;

  // conversion to a pointer type.
  Holder h{99};
  if (*h.operator int*() != 99) return 8;

  // The implicit (operator) forms must still work too.
  if (a(5) != 15) return 9;
  if (a + 7 != 17) return 10;
  if (a[3] != 13) return 11;

  return 0;
}
