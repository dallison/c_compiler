// Reference non-type template parameters on function and class templates bind
// to the named object, not to a copy of its value.
constexpr int ka = 4;
constexpr int kb = 4;
int g = 9;
int h = 10;
int arr[3] = {1, 2, 3};
struct S {
  int x;
};
S s{5};
constexpr S cs{6};

template <const int& R>
const int* Where() { return &R; }

template <int& R>
int* Addr() { return &R; }

template <int& R>
struct Counter {
  static int Bump() { return ++R; }
};

template <const int& R>
struct Box {
  const int* Get() const { return &R; }
};

template <int (&A)[3]>
int Sum() { return A[0] + A[1] + A[2]; }

template <S& O>
int GetX() { return O.x; }

template <const S& O>
int GetCX() { return O.x; }

template <const int& R>
constexpr int Twice() { return R * 2; }

static_assert(Twice<ka>() == 8);

template <class T>
T* AddrIn() { return Addr<g>(); }

int main() {
  if (Where<ka>() != &ka || Where<kb>() != &kb) return 1;
  if (Where<ka>() == Where<kb>()) return 2;
  if (Addr<g>() != &g || AddrIn<int>() != &g) return 3;
  *Addr<g>() = 3;
  if (g != 3) return 4;
  if (Counter<g>::Bump() != 4 || Counter<h>::Bump() != 11) return 5;
  if (g != 4 || h != 11) return 6;
  if (Box<ka>{}.Get() != &ka) return 7;
  if (Sum<arr>() != 6) return 8;
  if (GetX<s>() != 5 || GetCX<cs>() != 6) return 9;
  static int local = 3;
  if (Counter<local>::Bump() != 4 || local != 4) return 10;
  return 0;
}
