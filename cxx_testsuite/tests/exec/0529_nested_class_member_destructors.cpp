// RUN: -std=c++20
// EXPECT_EXIT: 0

// A nested class or typedef is a member of its enclosing class for name lookup
// but occupies no storage, so the enclosing destructor must not try to destroy
// one.  Out-of-line destructor definitions used to do exactly that.

int destroyed;

struct Outer {
  struct Polymorphic {
    int v;
    virtual ~Polymorphic() { destroyed += 1; }
  };

  struct Plain {
    int t;
    ~Plain() { destroyed += 100; }
  };

  typedef Plain Alias;

  Outer();
  ~Outer();

  int* p;
};

Outer::Outer() : p(nullptr) {}

Outer::~Outer() { destroyed += 1000; }

int main(void) {
  {
    Outer o;
  }
  if (destroyed != 1000) {
    return 1;
  }

  // The nested types still work as types, and destroy normally in their own
  // right.
  destroyed = 0;
  {
    Outer::Plain plain;
    plain.t = 1;
  }
  if (destroyed != 100) {
    return 2;
  }

  destroyed = 0;
  {
    Outer::Alias alias;
    alias.t = 2;
  }
  if (destroyed != 100) {
    return 3;
  }

  destroyed = 0;
  {
    Outer::Polymorphic* poly = new Outer::Polymorphic;
    delete poly;
  }
  if (destroyed != 1) {
    return 4;
  }

  return 0;
}
