// A declaration used as the substatement of if/else/while/for is in a block
// scope of its own ([stmt.pre]); googletest expands `else static_assert(...)`.

struct Counted {
  static int destroyed;
  ~Counted() { destroyed++; }
};
int Counted::destroyed;

int f(int x) {
  if (x) {
    x++;
  } else
    static_assert(true, "");
  while (0) static_assert(1);
  for (int i = 0; i < 2; i++) static_assert(1, "x");
  return x;
}

int main() {
  if (f(1) != 2) return 1;
  for (int i = 0; i < 3; i++) Counted c;
  if (Counted::destroyed != 3) return 2;
  int x = 7;
  if (x == 7) int x = 1;  // shadows only within the substatement
  else int x = 2;
  if (x != 7) return 3;
  return 0;
}
