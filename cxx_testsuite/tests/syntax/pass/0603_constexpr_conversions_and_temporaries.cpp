// RUN: -std=c++20
// Constant evaluation of floating-integral conversions ([conv.fpint]: the
// value is truncated, not computed in integer arithmetic), conversions to
// bool ([conv.bool]), copy-initialization of a class from a converted value,
// and references bound to scalar temporaries ([class.temporary]).
#include <memory>
#include <variant>

constexpr int Cast() { double x = 2.5; return (int)(x * 2); }
constexpr int StaticCast() { double x = 2.5; return static_cast<int>(x * 2); }
constexpr int Implicit() { double x = 2.5; int r = x * 2; return r; }
constexpr int Returned() { double x = 2.5; return x * 2; }
constexpr int Assigned() { double x = 2.5; int r = 0; r = x * 2; return r; }
constexpr int Compound() { double x = 2.5; int r = 1; r += x * 2; return r; }
constexpr int Negative() { double x = -2.75; return (int)x; }
constexpr int FromFloat() { float f = 2.5f; return (int)(f * 2); }
constexpr unsigned long long Large() {
  double x = 1.5e19;
  return (unsigned long long)x;
}
static_assert(Cast() == 5);
static_assert(StaticCast() == 5);
static_assert(Implicit() == 5);
static_assert(Returned() == 5);
static_assert(Assigned() == 5);
static_assert(Compound() == 6);
static_assert(Negative() == -2);
static_assert(FromFloat() == 5);
static_assert(Large() == 15000000000000000000ull);

constexpr bool BoolCast() { double x = 0.25; return (bool)(x * 2); }
constexpr bool BoolInit() { double x = 0.25; bool b = x * 2; return b; }
constexpr int Condition() { double x = 0.25; if (x * 2) return 1; return 0; }
constexpr bool Not() { double x = 0.25; return !(x * 2); }
constexpr bool Zero() { double x = 0.5; return (bool)(x * 2 - 1); }
static_assert(BoolCast());
static_assert(BoolInit());
static_assert(Condition() == 1);
static_assert(!Not());
static_assert(!Zero());

constexpr int StoredThroughPointer() {
  double x = 0;
  double* p = &x;
  *p = 2.5;
  return (int)(x * 2);
}
constexpr int ConstructedAt() {
  double x = 0;
  std::construct_at(&x, 2.5);
  return (int)(x * 2);
}
static_assert(StoredThroughPointer() == 5);
static_assert(ConstructedAt() == 5);

struct Plain { int v; constexpr Plain(int x) : v(x) {} };
struct Owned {
  int v;
  constexpr Owned(int x) : v(x) {}
  constexpr ~Owned() {}
};
constexpr int Converted() { Plain p = 8; return p.v; }
constexpr int ConvertedOwned() { Owned o = 8; return o.v; }
constexpr int CopiedOwned() { Owned o = Owned(9); return o.v; }
static_assert(Converted() == 8);
static_assert(ConvertedOwned() == 8);
static_assert(CopiedOwned() == 9);

constexpr int RvalueRef() { int&& r = 2; return r * 2; }
constexpr int ConstRef() { const double& r = 2.5; return (int)(r * 2); }
constexpr int WrittenRef() { int&& r = 2; r = 5; return r; }
constexpr int AddressedRef() { int&& r = 2; int* p = &r; *p = 6; return r; }
constexpr int ExpressionRef() { const int& r = 3 + 4; return r; }
constexpr int ConvertedRef() {
  int x = 1;
  const long& r = x;
  x = 9;
  return (int)r;
}
constexpr int ForwardedRef() {
  double x = 0;
  double&& r = 2.5;
  std::construct_at(&x, static_cast<double&&>(r));
  return (int)(x * 2);
}
static_assert(RvalueRef() == 4);
static_assert(ConstRef() == 5);
static_assert(WrittenRef() == 5);
static_assert(AddressedRef() == 6);
static_assert(ExpressionRef() == 7);
static_assert(ConvertedRef() == 1);
static_assert(ForwardedRef() == 5);

constexpr int VariantDouble() {
  std::variant<int, double> v(2.5);
  return (int)(std::get<1>(v) * 2);
}
constexpr int VariantReassigned() {
  std::variant<int, double> a(2), b(a);
  b = 2.5;
  return (int)(std::get<1>(b) * 2) + std::get<0>(a);
}
constexpr int VariantEmplaced() {
  std::variant<int, double> v(2);
  v.emplace<1>(2.5);
  return (int)(std::get<1>(v) * 2) + (int)v.index();
}
struct Q { int v; constexpr Q(int x) : v(x) {} };
constexpr int VariantClassAssigned() {
  std::variant<int, Q> v(1);
  v = Q(9);
  return std::get<Q>(v).v;
}
static_assert(VariantDouble() == 5);
static_assert(VariantReassigned() == 7);
static_assert(VariantEmplaced() == 6);
static_assert(VariantClassAssigned() == 9);

int main() {
  return Cast() + Implicit() + VariantReassigned() + RvalueRef() ==
                 5 + 5 + 7 + 4
             ? 0
             : 1;
}
