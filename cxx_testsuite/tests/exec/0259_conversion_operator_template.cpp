// A conversion operator can be a template (`template<class T> operator T()`).
// Its template arguments are deduced from the required target type at the point
// of conversion ([temp.deduct.conv]); the deduced specialization then behaves
// like an ordinary conversion operator, including a trailing standard
// conversion and contextual conversions.

struct Any {
  int v;
  template <class T>
  operator T() const {
    return (T)v;
  }
};

struct PtrHolder {
  int storage;
  // A conversion operator template whose target is a pointer: T is deduced from
  // the pointee of the requested pointer type.
  template <class T>
  operator T*() {
    return (T*)&storage;
  }
};

static int take_int(int x) { return x; }
static double take_double(double d) { return d; }
static long take_long(long x) { return x; }

int main() {
  Any a{42};

  // Deduce T = int / long / double from the initializer target type.
  int i = a;
  if (i != 42) return 1;
  long l = a;
  if (l != 42) return 2;
  double d = a;
  if (d != 42.0) return 3;

  // Deduce from the function-parameter target type.
  if (take_int(a) != 42) return 4;
  if (take_long(a) != 42) return 5;
  if (take_double(a) != 42.0) return 6;

  // A user-defined conversion (operator char) followed by a standard
  // conversion char -> int.
  Any small{7};
  char c = small;
  if (c != 7) return 7;

  // Contextual bool: deduce operator bool from the boolean context.
  Any zero{0};
  Any nonzero{5};
  if (zero) return 8;
  if (!nonzero) return 9;

  // Explicit cast selects the requested specialization.
  if (static_cast<long>(a) != 42) return 10;

  // Pointer conversion operator template: deduce T = int from int*.
  PtrHolder p{99};
  int* ip = p;
  if (*ip != 99) return 11;
  *ip = 123;
  if (p.storage != 123) return 12;

  return 0;
}
