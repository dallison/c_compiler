// A constexpr constructor that copies a base (`D(C c) : C(c)`) whose member
// has an anonymous union copies into the base subobject of the object being
// built.  Treating the derived object as the copy destination recursed without
// end.  The copied values, including the anonymous union's active member, must
// survive into the objects' constant images.
struct M {
  constexpr M(int v) : a(v), b(v * 2) {}
  int a;
  union {
    void* f;
    unsigned long b;
  };
};
struct C {
  M m;
  int k;
  constexpr C(M m, int k) : m(m), k(k) {}
};
struct D : C {
  int z;
  constexpr D(C c, int z) : C(c), z(z) {}
};
struct E : D {
  constexpr E(D d) : D(d) {}
};

constexpr D d{C{M{5}, 7}, 9};
static_assert(d.m.a == 5 && d.m.b == 10 && d.k == 7 && d.z == 9, "");
constexpr E e{D{C{M{3}, 4}, 6}};
static_assert(e.m.a == 3 && e.m.b == 6 && e.k == 4 && e.z == 6, "");
extern const E ge;
const E ge{D{C{M{1}, 2}, 3}};

template <typename T>
const T* Launder(const T* p) {
  const T* volatile q = p;
  return q;
}

int main() {
  const D* pd = Launder(&d);
  if (pd->m.a != 5 || pd->m.b != 10 || pd->k != 7 || pd->z != 9) return 1;
  const E* pe = Launder(&e);
  if (pe->m.a != 3 || pe->m.b != 6 || pe->k != 4 || pe->z != 6) return 2;
  const E* pg = Launder(&ge);
  if (pg->m.a != 1 || pg->m.b != 2 || pg->k != 2 || pg->z != 3) return 3;
  return 0;
}
