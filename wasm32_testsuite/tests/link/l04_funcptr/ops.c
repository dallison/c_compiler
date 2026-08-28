typedef int (*Op)(int, int);

int multiply(int a, int b);

static int plus(int a, int b) { return a + b; }
static int minus(int a, int b) { return a - b; }

Op operations[3] = {plus, minus, multiply};

int apply(Op op, int a, int b) { return op(a, b); }
