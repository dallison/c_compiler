typedef __builtin_va_list va_list;

static volatile int global_value = 7;

static int add(int a, int b) { return a + b; }

static int arithmetic(int value) {
  return ((value * 9) / 3) + (value % 3) - 1;
}

static int branches(int limit) {
  int total = 0;
  for (int i = 0; i < limit; ++i) {
    if ((i & 1) == 0) {
      total += i;
    } else {
      total -= i;
    }
  }
  return total;
}

static int eight_args(int a, int b, int c, int d, int e, int f, int g, int h) {
  return a + b + c + d + e + f + g + h;
}

static int recurse(int depth) {
  if (depth == 0) {
    return 1;
  }
  return depth + recurse(depth - 1);
}

static int apply(int (*function)(int, int), int a, int b) {
  return function(a, b);
}

static int sum_varargs(int count, ...) {
  va_list args;
  __builtin_va_start(args, count);
  int total = 0;
  for (int i = 0; i < count; ++i) {
    total += __builtin_va_arg(args, int);
  }
  __builtin_va_end(args);
  return total;
}

struct Pair {
  int first;
  int second;
};

static struct Pair make_pair(int first, int second) {
  struct Pair pair = {first, second};
  return pair;
}

static long long wide_add(long long a, long long b) { return a + b; }

static int dense_switch(int value) {
  switch (value) {
    case 0: return 5;
    case 1: return 7;
    case 2: return 11;
    case 3: return 13;
    case 4: return 17;
    case 5: return 19;
    case 6: return 23;
    case 7: return 29;
    default: return -1;
  }
}

int main(void) {
  unsigned char bytes[4] = {1, 2, 3, 4};
  unsigned short halves[2] = {100, 200};
  struct Pair pair = make_pair(19, 23);

  if (add(19, 23) != 42) return 1;
  if (arithmetic(14) != 43) return 2;
  if (branches(7) != 3) return 3;
  if (global_value != 7) return 4;
  global_value = 35;
  if (global_value + 7 != 42) return 5;
  if (bytes[0] + bytes[1] + bytes[2] + bytes[3] != 10) return 6;
  if (halves[0] + halves[1] != 300) return 7;
  if (eight_args(1, 2, 3, 4, 5, 6, 7, 14) != 42) return 8;
  if (recurse(12) != 79) return 9;
  if (apply(add, 20, 22) != 42) return 10;
  if (sum_varargs(4, 9, 10, 11, 12) != 42) return 11;
  if (pair.first + pair.second != 42) return 12;
  if (wide_add(0x100000002ll, 0x200000003ll) != 0x300000005ll) return 13;
  if (dense_switch(6) + dense_switch(5) != 42) return 14;
  return 0;
}
