// RUN: -std=c++11
// Non-narrowing scalar list-initializations must remain well-formed: exact
// values, widening conversions, in-range constants, and explicit casts are all
// fine.  Guards the narrowing check against false positives.

int main() {
  int a{5};
  long b{70000000000};
  char c{100};
  unsigned u{7};
  double d{3.5};
  long e{5};
  int f{(int)e};
  double g{2};
  return a + (int)b + c + (int)u + (int)d + (int)e + f + (int)g;
}
