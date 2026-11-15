int ext(int);
double ext_double(double);

int count(int n) {
  int total = 0;
  for (int i = 0; i < n; i++) {
    total += ext(i);
  }
  return total;
}

double scale(double x, int n) {
  double acc = 1.0;
  for (int i = 0; i < n; i++) {
    acc = ext_double(acc) * x;
  }
  return acc;
}

struct Big {
  long a[6];
};

struct Big make(int n) {
  struct Big r;
  for (int i = 0; i < 6; i++) {
    r.a[i] = ext(n + i);
  }
  return r;
}
