// Inside a member of a class with a private base, a pointer to the class
// converts to a pointer to that base, so calling the base's member template
// with explicit template arguments and the derived pointer is viable.
// Overload resolution must rank that conversion like any derived-to-base
// conversion, which is better than a conversion to `void*`.
struct Arena;
struct Base {
  int n = 0;
  Arena* GetArena() const { return nullptr; }
  template <typename H> void Swap(Arena*, Base* other, Arena*) {
    int t = n;
    n = other->n;
    other->n = t;
  }
  template <typename H> void SwapWith(Base* other) { Swap<H>(nullptr, other, nullptr); }
};
template <typename T> struct Handler {};

template <typename E> struct Field final : private Base {
  using TypeHandler = Handler<E>;
  void Swap(Field* other);
  void SwapUnqualified(Field* other) { SwapWith<TypeHandler>(other); }
  void SwapThroughThis(Field* other) { this->SwapWith<int>(other); }
  void set(int v) { n = v; }
  int get() const { return n; }
};
template <typename E> inline void Field<E>::Swap(Field* other) {
  if (this == other) return;
  Base::Swap<TypeHandler>(GetArena(), other, other->GetArena());
}

int Pick(Base*) { return 1; }
int Pick(void*) { return 2; }
struct Derived : private Base {
  int PickSelf() { return Pick(this); }
};
struct PublicDerived : Base {};

struct Msg {};

int main() {
  Field<Msg> a, b;
  a.set(1);
  b.set(2);
  a.Swap(&b);
  if (a.get() != 2 || b.get() != 1) return 1;
  a.SwapUnqualified(&b);
  if (a.get() != 1 || b.get() != 2) return 2;
  a.SwapThroughThis(&b);
  if (a.get() != 2 || b.get() != 1) return 3;
  Derived d;
  if (d.PickSelf() != 1) return 4;
  PublicDerived p;
  if (Pick(&p) != 1) return 5;
  return 0;
}
