long passthrough(long x) { return x; }

struct Table {
  long* slots;
  long size;
};

struct Holder {
  struct Table table;
};

void table_init(struct Table* table, long size);

void holder_init(struct Holder* holder, long size) {
  table_init(&holder->table, size);
}

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

long ext_long(long);

long many(long n) {
  long a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8;
  long i = 9, j = 10, k = 11, l = 12, m = 13, o = 14;
  for (long t = 0; t < n; t++) {
    a += ext_long(a);
    b += ext_long(b);
    c += ext_long(c);
    d += ext_long(d);
    e += ext_long(e);
    f += ext_long(f);
    g += ext_long(g);
    h += ext_long(h);
    i += ext_long(i);
    j += ext_long(j);
    k += ext_long(k);
    l += ext_long(l);
    m += ext_long(m);
    o += ext_long(o);
  }
  return a * b + c * d + e * f + g * h + i * j + k * l + m * o;
}
