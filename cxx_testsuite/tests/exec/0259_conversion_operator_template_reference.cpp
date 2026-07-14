// A conversion operator template whose target is a reference (`operator T&`)
// deduces T from the referent of the requested reference type and binds the
// result directly as an lvalue (no trailing standard conversion is applied to a
// reference target).

struct Box {
  int v;
  template <class T>
  operator T&() {
    return *reinterpret_cast<T*>(&v);
  }
};

struct ConstBox {
  mutable int v;
  template <class T>
  operator const T&() const {
    return *reinterpret_cast<const T*>(&v);
  }
};

static int read_cref(const int& x) { return x; }
static void bump_lref(int& x) { x += 100; }

int main() {
  Box b{7};

  // Bind to a non-const lvalue reference: T = int.  Mutating through the
  // reference writes back to the object.
  int& r = b;
  if (r != 7) return 1;
  r = 42;
  if (b.v != 42) return 2;

  // Bind to a const lvalue reference: still T = int.
  const int& cr = b;
  if (cr != 42) return 3;

  // Deduce the reference target from a function parameter.
  bump_lref(b);
  if (b.v != 142) return 4;
  if (read_cref(b) != 142) return 5;

  // A const-qualified conversion operator template returning `const T&`.
  ConstBox cb{9};
  const int& ccr = cb;
  if (ccr != 9) return 6;
  if (read_cref(cb) != 9) return 7;

  return 0;
}
