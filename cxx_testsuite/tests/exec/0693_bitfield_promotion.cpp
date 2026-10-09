// Integral promotion of bit-fields ([conv.prom]/5): a field whose values all
// fit in int promotes to int, even when its declared type is unsigned int or
// unsigned long long; an unsigned field as wide as int promotes to unsigned
// int.
struct S {
  unsigned c : 4;
  unsigned w : 32;
  unsigned long long l : 4;
  int s : 3;
};

__attribute__((noinline)) S Make() { return S{3, 3, 3, 1}; }

constexpr S k{3, 3, 3, 1};

int main() {
  S s = Make();
  if (!(s.c - 5 < 0)) return 1;
  if (!(-s.c < 0)) return 2;
  if (!(~s.c < 0)) return 3;
  if (!(s.w - 5 > 0)) return 4;
  if (!(s.l - 5 < 0)) return 5;
  if (sizeof(+s.c) != sizeof(int)) return 6;
  if (!(s.c * -1 < 0)) return 7;
  if (!(s.c + s.s - 10 < 0)) return 8;
  if (!(((s.c << 1) - 7) < 0)) return 9;
  int folded = k.c - 5;
  if (folded != -2) return 10;
  if (!(k.c - 5 < 0)) return 11;
  return 0;
}
