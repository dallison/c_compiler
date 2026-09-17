// RUN: -std=c++20
// EXPECT_EXIT: 0

// Inside a constructor or destructor a virtual call on `this` is bound to the
// class being built or torn down, because the vptr names that class for the
// duration of the body.  A call on any other object is an ordinary virtual
// call and dispatches on that object's dynamic type.

int dtor_result;

struct Shape {
  virtual ~Shape() {}
  virtual int sides() { return 0; }
};

struct Square : Shape {
  int sides() override { return 4; }
};

struct Probe : Shape {
  Shape* other;
  int other_in_ctor;
  int this_in_ctor;

  explicit Probe(Shape* s) : other(s) {
    other_in_ctor = s->sides();
    this_in_ctor = sides();
    this_in_ctor += (*this).sides();
  }

  ~Probe() override { dtor_result = other->sides(); }

  int sides() override { return 1; }
};

struct DerivedProbe : Probe {
  explicit DerivedProbe(Shape* s) : Probe(s) {}
  int sides() override { return 99; }
};

// A second base puts the Probe subobject at a non-zero offset, so the base
// constructor and destructor calls reach it through an adjusted `this`.
struct Tag {
  virtual ~Tag() {}
  virtual int tag() { return 8; }
};

struct MultiProbe : Tag, Probe {
  explicit MultiProbe(Shape* s) : Probe(s) {}
  int sides() override { return 77; }
  int tag() override { return 9; }
};

int main(void) {
  Square square;

  {
    Probe probe(&square);
    if (probe.other_in_ctor != 4) {
      return 1;
    }
    if (probe.this_in_ctor != 2) {
      return 2;
    }
  }
  if (dtor_result != 4) {
    return 3;
  }

  dtor_result = 0;
  {
    DerivedProbe probe(&square);
    if (probe.other_in_ctor != 4) {
      return 4;
    }
    if (probe.this_in_ctor != 2) {
      return 5;
    }
    if (probe.sides() != 99) {
      return 6;
    }
  }
  if (dtor_result != 4) {
    return 7;
  }

  dtor_result = 0;
  {
    MultiProbe probe(&square);
    if (probe.other_in_ctor != 4) {
      return 8;
    }
    if (probe.this_in_ctor != 2) {
      return 9;
    }
    if (probe.sides() != 77 || probe.tag() != 9) {
      return 10;
    }
  }
  if (dtor_result != 4) {
    return 11;
  }

  return 0;
}
