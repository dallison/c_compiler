// `obj.Base::member` names the member of that base subobject: plain bases,
// specializations of one template (`B<0>`, `B<1>`, ...), a member template's
// parameter as the qualifier, a namespace-qualified base, and virtual bases.
#include <cstddef>

namespace ns {
struct Tagged { int tag; };
}

struct A { int a; };
struct B { int value; };
struct C {
  int value;
  C& operator=(int v) { value = v; return *this; }
};
struct D : A, B, C, ns::Tagged {
  int ReadC() { return C::value; }
  int ReadB() const { return this->B::value; }
};

template <std::size_t I> struct Slot { int value; };
struct Slots : Slot<0>, Slot<1>, Slot<2> {
  template <std::size_t J> int& Get() { return Slot<J>::value; }
  template <std::size_t J> int& GetCast() {
    return static_cast<Slot<J>*>(this)->value;
  }
};
template <class T> struct SlotsT : Slot<0>, Slot<1>, Slot<2> {
  template <std::size_t J> int& Get() { return Slot<J>::value; }
};

struct V { int v; };
struct VL : virtual V { int l; };
struct VR : virtual V { int r; };
struct VD : VL, VR { int d; };

int main() {
  D d;
  d.a = 1;
  static_cast<B&>(d).value = 2;
  static_cast<C&>(d).value = 3;
  static_cast<ns::Tagged&>(d).tag = 4;
  if (d.B::value != 2) return 1;
  if (d.C::value != 3) return 2;
  if (d.ReadC() != 3 || d.ReadB() != 2) return 3;
  if (d.ns::Tagged::tag != 4) return 4;
  D* p = &d;
  p->C::value = 30;
  if (static_cast<C&>(d).value != 30 || static_cast<B&>(d).value != 2) return 5;
  d.C::operator=(31);
  if (static_cast<C&>(d).value != 31 || d.a != 1) return 6;

  Slots s;
  s.Slot<0>::value = 10;
  s.Slot<1>::value = 11;
  s.Slot<2>::value = 12;
  if (static_cast<Slot<0>&>(s).value != 10) return 7;
  if (static_cast<Slot<1>&>(s).value != 11) return 8;
  if (static_cast<Slot<2>&>(s).value != 12) return 9;
  if (s.Get<0>() != 10 || s.Get<1>() != 11 || s.Get<2>() != 12) return 10;
  if (s.GetCast<1>() != 11 || s.GetCast<2>() != 12) return 11;
  s.Get<1>() = 21;
  if (s.Slot<1>::value != 21 || s.Slot<0>::value != 10) return 12;

  SlotsT<int> t;
  static_cast<Slot<0>&>(t).value = 40;
  static_cast<Slot<1>&>(t).value = 41;
  static_cast<Slot<2>&>(t).value = 42;
  if (t.Get<2>() != 42 || t.Get<1>() != 41 || t.Get<0>() != 40) return 13;

  VD vd;
  vd.d = 5;
  static_cast<V&>(vd).v = 6;
  if (vd.V::v != 6) return 14;
  if (vd.VL::v != 6 || vd.VR::v != 6) return 15;
  vd.VR::v = 7;
  if (static_cast<V&>(vd).v != 7 || vd.d != 5) return 16;
  return 0;
}
