// A C++ assignment expression is an lvalue naming its left operand, so a
// reference bound to it, including a `T&` returned as `return l = r;`, must
// refer to that object rather than to a copy of the stored value.
enum Opt { kA = 1, kB = 2 };
struct Pair {
  int a;
  int b;
};

int g_value;
int& AssignParam(int& l) { return l = 5; }
int& AssignGlobal() { return g_value = 6; }
int& AddAssign(int& l) { return l += 5; }
int& AssignThrough(int* p) { return *p = 7; }
Opt& OrAssign(Opt& l, Opt r) { return l = static_cast<Opt>(l | r); }
Pair& AssignPair(Pair& l, const Pair& r) { return l = r; }
double& AssignDouble(double& l) { return l = 2.5; }
__int128& AssignWide(__int128& l) { return l = static_cast<__int128>(1) << 70; }

int main() {
  int x = 1;
  int& local = (x = 3);
  if (&local != &x || local != 3) return 1;
  if (&AssignParam(x) != &x || x != 5) return 2;
  if (&AssignGlobal() != &g_value || g_value != 6) return 3;
  int y = 1;
  if (&AddAssign(y) != &y || y != 6) return 4;
  int z = 0;
  if (&AssignThrough(&z) != &z || z != 7) return 5;
  Opt o = kA;
  if (&OrAssign(o, kB) != &o || o != 3) return 6;
  Pair p = {1, 2};
  Pair q = {3, 4};
  if (&AssignPair(p, q) != &p || p.b != 4) return 7;
  double d = 0;
  if (&AssignDouble(d) != &d || d != 2.5) return 8;
  __int128 w = 0;
  if (&AssignWide(w) != &w || (w >> 70) != 1) return 9;
  AssignParam(x) = 11;
  if (x != 11) return 10;
  (x = 1) += 2;
  if (x != 3) return 11;
  int a = 0, b = 0;
  a = b = 4;
  if (a != 4 || b != 4) return 12;
  return 0;
}
