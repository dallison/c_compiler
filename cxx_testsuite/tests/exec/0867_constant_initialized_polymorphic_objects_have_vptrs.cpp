// A constant-initialized polymorphic object is emitted as data, so its image
// must carry the vptrs the constructor would have stored: for a complete
// object, a base subobject (whose vptr is the most-derived class's), a member,
// an array element, and a class template specialization.  Each is used by an
// initializer that runs before its definition, when only the emitted data
// exists.
struct B {
  constexpr B(int x) : x(x) {}
  virtual int K() const { return x; }
  int x;
};
struct D : B {
  constexpr D() : B(4) {}
  int K() const override { return 9; }
};
struct H {
  constexpr H() : d() {}
  union {
    D d;
  };
};
struct H2 {
  constexpr H2() : d() {}
  int pad = 1;
  D d;
};
struct Arr {
  constexpr Arr() : arr{B(4), B(5)} {}
  int x = 1;
  B arr[2];
};
template <typename T>
struct TT {
  constexpr TT(T t) : t(t) {}
  virtual T Get() const { return t * 2; }
  T t;
};

constexpr D cd;
static_assert(cd.x == 4, "");

extern D d;
extern H h;
extern H2 h2;
extern Arr arr;
extern TT<long> tt;
static const B* volatile pd = &d;
static const B* volatile ph = &h.d;
static const B* volatile ph2 = &h2.d;
static const B* volatile parr = &arr.arr[1];
static const TT<long>* volatile ptt = &tt;
static int e1 = pd->K();
static int e2 = ph->K();
static int e3 = ph2->K();
static int e4 = parr->K();
static long e5 = ptt->Get();
D d;
H h;
H2 h2;
Arr arr;
TT<long> tt{21};

int main() {
  if (e1 != 9) return 1;
  if (e2 != 9) return 2;
  if (e3 != 9) return 3;
  if (e4 != 5) return 4;
  if (e5 != 42) return 5;
  const B* volatile pcd = &cd;
  if (pcd->K() != 9) return 6;
  return 0;
}
