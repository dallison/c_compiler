// The x86-64 target optimizer folds an add of a constant into the offset of the
// load that used it.  When the add also wrote its result into a register
// variable, folding removed the load's use and the add was deleted with it, so
// the variable was never written and a later read of it had no register at all.

struct Cookie {
  long count;
  int values[4];
};

static long sum_from_cookie(int* array_body) {
  // `base` is read once here, to reach the count that precedes the elements,
  // and once at the end, so it stays live past the fold.
  char* base = reinterpret_cast<char*>(array_body) - sizeof(long);
  long count = *reinterpret_cast<long*>(base);
  long total = 0;
  for (long i = 0; i < count; i++) {
    total += array_body[i];
  }
  return total + reinterpret_cast<long*>(base)[0];
}

int main(void) {
  Cookie cookie;
  cookie.count = 4;
  cookie.values[0] = 1;
  cookie.values[1] = 2;
  cookie.values[2] = 3;
  cookie.values[3] = 4;
  if (sum_from_cookie(cookie.values) != 14) {
    return 1;
  }
  cookie.count = 2;
  if (sum_from_cookie(cookie.values) != 5) {
    return 2;
  }
  return 0;
}
