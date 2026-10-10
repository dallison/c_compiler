// A static data member named through an object expression (`f().value`,
// `p->value`) is the static member itself, not a slot of the object.
template <class T, T v>
struct Constant {
  static constexpr T value = v;
};

struct Base {
  static constexpr int kBase = 5;
};

struct Derived : Base {
  int field = 1;
  static constexpr int kDerived = 9;
};

template <class A>
constexpr Constant<int, sizeof(A)> SizeOf() { return {}; }

constexpr Derived MakeDerived() { return {}; }

template <class U>
int ThroughCall() { return SizeOf<U>().value; }

constexpr int ThroughPointer() {
  Derived d;
  const Derived* p = &d;
  return p->kDerived + p->kBase + p->field;
}

static_assert(SizeOf<long long>().value == 8, "");
static_assert(MakeDerived().kBase == 5, "");
static_assert(MakeDerived().kDerived == 9, "");
static_assert(ThroughPointer() == 15, "");

int main() {
  if (ThroughCall<long long>() != 8) return 1;
  if (ThroughCall<char>() != 1) return 2;
  int v = SizeOf<short>().value;
  if (v != 2) return 3;
  constexpr int c = MakeDerived().kDerived;
  if (c != 9) return 4;
  if (ThroughPointer() != 15) return 5;
  return 0;
}
