// Structs by value across objects, which on wasm means a pointer to a copy
// the caller made and, for the return, a hidden pointer the caller supplies.

struct Point {
  int x;
  int y;
};

struct Big {
  int values[8];
  char tag;
};

struct Point make(int x, int y);
int sum(struct Point p);
struct Big grow(struct Big b);

int main(void) {
  struct Point p = make(3, 4);
  int total = sum(p);

  struct Big b;
  for (int i = 0; i < 8; i++) {
    b.values[i] = i;
  }
  b.tag = 5;
  struct Big g = grow(b);
  for (int i = 0; i < 8; i++) {
    total += g.values[i];
  }
  return total + g.tag;
}
