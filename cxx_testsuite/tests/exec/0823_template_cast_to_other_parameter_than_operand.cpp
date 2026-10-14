// In `static_cast<const U*>(p)` with `O* p`, the target names U, not the
// operand's parameter O.
struct A {
  int a = 1;
};
struct B : A {
  int b = 2;
};
struct C : B {
  int c = 3;
};

template <class U, class O>
U* Up(O* p) {
  return const_cast<U*>(static_cast<const U*>(p));
}

template <class U, class O>
int SizeOfTarget(O* p) {
  auto r = static_cast<const U*>(p);
  return sizeof(*r);
}

template <class U, class O>
U& UpRef(O& o) {
  return static_cast<U&>(o);
}

int main() {
  C c;
  if (Up<A>(&c)->a != 1) return 1;
  if (Up<B>(&c)->b != 2) return 2;
  if (SizeOfTarget<A>(&c) != sizeof(A)) return 3;
  if (SizeOfTarget<B>(&c) != sizeof(B)) return 4;
  if (UpRef<B>(c).b != 2) return 5;
  return 0;
}
