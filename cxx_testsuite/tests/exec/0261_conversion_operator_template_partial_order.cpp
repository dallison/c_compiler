// When several conversion operator templates are viable for a target type, the
// most specialized one is selected by partial ordering ([temp.func.order]).
// Here `operator T*` is more specialized than `operator T`, so a conversion to
// a pointer type picks the pointer operator, while a conversion to a
// non-pointer type can only use the general one.

struct S {
  int storage;
  // #1: the general form -- viable for any target type.
  template <class T>
  operator T() {
    return (T)storage;
  }
  // #2: more specialized -- only viable when the target is a pointer.
  template <class T>
  operator T*() {
    return (T*)&storage;
  }
};

static int deref(int* p) { return *p; }

int main() {
  S s{5};

  // Non-pointer target: only #1 deduces (T = int).
  int v = s;
  if (v != 5) return 1;
  long lv = s;
  if (lv != 5) return 2;

  // Pointer target: both #1 (T = int*) and #2 (T = int) deduce; #2 is more
  // specialized and wins, yielding a pointer to the object's storage.
  int* p = s;
  if (p != &s.storage) return 3;
  *p = 9;
  if (s.storage != 9) return 4;

  // Same selection through a function-parameter target type.
  if (deref(s) != 9) return 5;

  return 0;
}
