struct Point {
  int x;
  int y;
};

struct Big {
  int values[8];
  char tag;
};

struct Point make(int x, int y) {
  struct Point p;
  p.x = x;
  p.y = y;
  return p;
}

int sum(struct Point p) { return p.x + p.y; }

struct Big grow(struct Big b) {
  for (int i = 0; i < 8; i++) {
    b.values[i] *= 2;
  }
  b.tag += 1;
  return b;
}
