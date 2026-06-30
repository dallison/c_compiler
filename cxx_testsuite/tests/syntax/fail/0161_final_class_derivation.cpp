// RUN: -std=c++20
// A class declared 'final' cannot be used as a base class.  This applies to a
// final class with no base clause as well as one that itself derives.
// EXPECT: cannot derive from final base class Sealed
// EXPECT: cannot derive from final base class SealedDerived

struct Sealed final {
  int v;
};

struct DeriveFromSealed : public Sealed {
  int w;
};

struct Base {
  int b;
};

struct SealedDerived final : public Base {
  int s;
};

struct DeriveFromSealedDerived : public SealedDerived {
  int d;
};
