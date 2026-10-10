// A constexpr reference may bind to any object with static storage duration,
// const or not.  It designates that object: writes through it are visible,
// and reads in constant expressions see a const object's value.
int g = 5;
int arr[3] = {1, 2, 3};
struct P {
  int x;
  int y;
} pg{1, 2};
const int kc = 6;
struct S {
  int a;
  double b;
};
constexpr S s{3, 2.5};
long gl = 40;

constexpr int& r1 = g;
constexpr int& r2 = arr[1];
constexpr int& r3 = pg.y;
constexpr const int& r4 = g;
constexpr const int& rc = kc;
constexpr const S& rs = s;
constexpr const int& rsa = s.a;
constexpr const double& rsb = s.b;
constexpr long& rl = gl;

static_assert(&r1 == &g);
static_assert(&r2 == &arr[1]);
static_assert(&r3 == &pg.y);
static_assert(&r4 == &g);
static_assert(rc == 6 && &rc == &kc);
static_assert(rs.a == 3 && &rs == &s);
static_assert(rsa == 3 && &rsa == &s.a);
static_assert(rsb == 2.5);

struct C {
  static constexpr const int& m = kc;
  static constexpr int& n = g;
};
static_assert(C::m == 6 && &C::n == &g);
template <class T>
struct TC {
  static constexpr const T& m = kc;
};
static_assert(TC<int>::m == 6);

int Locals() {
  static constexpr int& lr = g;
  constexpr int& ar = g;
  constexpr const int& ac = kc;
  static_assert(ac == 6);
  lr = 8;
  ar += 1;
  return g;
}

int main() {
  if (Locals() != 9 || C::n != 9 || C::m != 6 || TC<int>::m != 6) return 8;
  r1 = 7;
  if (g != 7) return 1;
  r2 = 9;
  if (arr[1] != 9) return 2;
  r3 = 4;
  if (pg.y != 4) return 3;
  g = 11;
  if (r4 != 11 || r1 != 11) return 4;
  if (rc != 6 || rs.a != 3 || rsa != 3 || rsb != 2.5) return 5;
  if (&rs != &s || &rsa != &s.a) return 6;
  rl += 2;
  if (gl != 42) return 7;
  return 0;
}
