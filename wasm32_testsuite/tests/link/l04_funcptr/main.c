// A function pointer taken in one object and called in another, and a table
// of them built by a static initializer.  Both need a table slot that only
// the link can assign.

typedef int (*Op)(int, int);

int apply(Op op, int a, int b);
extern Op operations[3];

static int subtract(int a, int b) { return a - b; }

int multiply(int a, int b) { return a * b; }

int main(void) {
  int total = apply(subtract, 20, 5);
  total += apply(multiply, 3, 4);
  for (int i = 0; i < 3; i++) {
    total += operations[i](10, 2);
  }
  return total;
}
