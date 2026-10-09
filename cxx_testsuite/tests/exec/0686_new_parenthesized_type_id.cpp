// RUN: -std=c++20
// EXPECT_EXIT: 0
// new-expressions naming their type with a parenthesized type-id
// ([expr.new]): `new (int)`, `new (int[4])` (an array new), and declarators
// that need parentheses such as `new (int (*)[3])`, with or without an
// initializer and after placement arguments.
#include <new>

struct S {
  int a = 3;
  int b;
  S() : b(4) {}
  S(int x, int y) : a(x), b(y) {}
};

int Check() {
  int* q = new (int)(5);
  if (*q != 5) return 1;
  delete q;

  S* s = new (S);
  if (s->a != 3 || s->b != 4) return 2;
  delete s;
  S* t = new (S)(7, 8);
  if (t->a != 7 || t->b != 8) return 3;
  delete t;

  int* p = new (int[4]){1, 2, 3, 4};
  if (p[0] + p[3] != 5) return 4;
  delete[] p;

  int** pp = new (int*[2]){nullptr, q};
  if (pp[0] != nullptr || pp[1] != q) return 5;
  delete[] pp;

  int (*pa)[3] = new (int[2][3]);
  pa[1][2] = 6;
  if (pa[1][2] != 6) return 6;
  delete[] pa;

  int row[3] = {7, 8, 9};
  int (**rp)[3] = new (int (*)[3])(&row);
  if ((**rp)[2] != 9) return 7;
  delete rp;

  alignas(S) unsigned char buffer[sizeof(S)];
  S* placed = new (buffer) (S)(1, 2);
  if (placed->a != 1 || placed->b != 2) return 8;
  placed->~S();

  int* n = new (int);
  *n = 10;
  int r = *n;
  delete n;
  return r == 10 ? 0 : 9;
}

int main() { return Check(); }
