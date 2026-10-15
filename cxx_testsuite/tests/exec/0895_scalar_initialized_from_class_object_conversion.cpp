// A scalar initialized from a class object calls the conversion function,
// at namespace scope as well as in a block, and `int n(Count{7})` is a
// variable: a type name followed by `{` cannot begin a parameter declaration.
namespace ns {
struct C {
  int n;
  explicit operator int() const { return n; }
};
}  // namespace ns

struct Count {
  int n;
  explicit operator int() const { return n; }
};

struct Imp {
  int n;
  operator int() const { return n; }
};

Count count{5};
Imp imp{6};
int direct_explicit(count);
long direct_implicit(imp);
int copy_implicit = imp;
int temporary_explicit(Count{7});
int temporary_implicit = Imp{8};
int scoped_temporary(ns::C{9});

int twice(int x) { return 2 * x; }

int main() {
  if (direct_explicit != 5) return 1;
  if (direct_implicit != 6) return 2;
  if (copy_implicit != 6) return 3;
  if (temporary_explicit != 7) return 4;
  if (temporary_implicit != 8) return 5;
  if (scoped_temporary != 9) return 6;

  int n(Count{10});
  int m(ns::C{11});
  int k(int{12});
  if (n != 10 || m != 11 || k != 12) return 7;

  int twice(int);
  void take(Count);
  int (*fp)(int) = twice;
  if (fp(3) != 6) return 8;
  return 0;
}
