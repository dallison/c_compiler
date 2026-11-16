// RUN: -std=c++20
// EXPECT_EXIT: 0

// A vector read through a pointer or out of an array is used as the object at
// that address, not as a value loaded from it: as an operand, an argument, a
// return value and an initializer.
typedef int int4 __attribute__((vector_size(16)));
typedef double double2 __attribute__((vector_size(16)));

__attribute__((noinline)) int4 add(int4 a, int4 b) { return a + b; }

__attribute__((noinline)) int4 element(int4* p, int i) { return p[i]; }

__attribute__((noinline)) int4 sum(int4* p, int n) {
  int4 acc = {0, 0, 0, 0};
  for (int i = 0; i < n; i++) {
    acc = add(acc, p[i]);
  }
  return acc;
}

__attribute__((noinline)) double2 scale(double2* p, int i) {
  return *(p + i) * p[0];
}

int main() {
  int4 v[3] = {{1, 2, 3, 4}, {10, 20, 30, 40}, {100, 200, 300, 400}};
  int4* p = v;
  int4 first = *p;
  if (first[0] != 1 || first[3] != 4) {
    return 1;
  }
  int4 second = p[1];
  if (second[0] != 10 || second[3] != 40) {
    return 2;
  }
  int4 third = element(v, 2);
  if (third[0] != 100 || third[3] != 400) {
    return 3;
  }
  int4 total = sum(v, 3);
  if (total[0] != 111 || total[1] != 222 || total[3] != 444) {
    return 4;
  }
  int4 mixed = add(v[1], p[2]) - *p;
  if (mixed[0] != 109 || mixed[3] != 436) {
    return 5;
  }
  double2 d[2] = {{2.0, 3.0}, {5.0, 7.0}};
  double2 s = scale(d, 1);
  if (s[0] != 10.0 || s[1] != 21.0) {
    return 6;
  }
  return 0;
}
