static int failures;

static void expect_ll(long long got, long long want) {
  if (got != want) {
    failures++;
  }
}

static void accept_zero(long long v) {
  expect_ll(v, 0);
}

static void accept_ll(long long v) {
  expect_ll(v, 42);
}

static void accept_two(long long a, long long b) {
  expect_ll(a, 7);
  expect_ll(b, 3);
}

static void accept_ptr_and_wide(int *p, long long v) {
  if (*p != 42) {
    failures++;
  }
  expect_ll(v, 5);
}

static void accept_three_wide(long long a, long long b, long long c) {
  expect_ll(a, 1);
  expect_ll(b, 2);
  expect_ll(c, 3);
}

static long long make_seven(void) {
  return 7;
}

static void accept_seven(long long v) {
  expect_ll(v, 7);
}

static void test_ternary_false_branch(void) {
  long long now = 5;
  long long deadline = 3;
  long long timeout = deadline > now ? deadline - now : 0;
  expect_ll(timeout, 0);
}

static void test_ternary_true_branch(void) {
  long long now = 3;
  long long deadline = 10;
  long long timeout = deadline > now ? deadline - now : 0;
  expect_ll(timeout, 7);
}

static void test_call_with_ternary_false(void) {
  long long now = 5;
  long long deadline = 3;
  accept_zero(deadline > now ? deadline - now : 0);
}

static void test_call_with_ternary_true(void) {
  long long now = 3;
  long long deadline = 10;
  accept_seven(deadline > now ? deadline - now : 0);
}

static void test_two_wide_merge_args(void) {
  long long x = 10;
  long long y = 3;
  long long a = 9;
  long long b = 6;
  accept_two(x > y ? x - y : 0, a > b ? a - b : 0);
}

static void test_second_register_pair(void) {
  int marker = 0;
  long long x = 8;
  long long y = 1;
  long long a = 4;
  long long b = 1;
  accept_two(x > y ? x - y : 0, a > b ? a - b : 0);
  (void)marker;
}

static void test_pointer_interleave(void) {
  int value = 42;
  long long a = 6;
  long long b = 1;
  accept_ptr_and_wide(&value, a > b ? a - b : 0);
}

static void test_libcall_producer(void) {
  long long n = 6;
  accept_ll(n > 0 ? n * 7 : 0);
}

static void test_wide_load_producer(void) {
  long long values[2];
  values[0] = 42;
  values[1] = 0;
  accept_ll(values[0] > 0 ? values[0] : 0);
}

static void test_signextend_producer(void) {
  short s = -1;
  accept_ll(s > 0 ? s : 42);
}

static void test_true_signextend_producer(void) {
  short s = -1;
  long long wide = s;
  accept_ll(wide > 0 ? wide : 42);
}

static void test_nested_ternary(void) {
  long long a = 10;
  long long b = 3;
  long long c = 8;
  long long d = 2;
  expect_ll(a > b ? (c > d ? c - d : 0) : 0, 6);
}

static int action_matches_stub(const int *header, long long type_filter,
                               const int *thrown, int *offset, int *is_catch,
                               int *is_cleanup) {
  (void)header;
  (void)thrown;
  *offset = 0;
  *is_catch = 0;
  *is_cleanup = 0;
  if (type_filter == 0) {
    *is_cleanup = 1;
    return 1;
  }
  if (type_filter < 0) {
    return 0;
  }
  *offset = (int)type_filter;
  *is_catch = 1;
  return 1;
}

static void test_action_matches_shape(void) {
  int header = 1;
  int thrown = 2;
  int offset = 0;
  int is_catch = 0;
  int is_cleanup = 0;
  long long x = 8;
  long long y = 1;
  long long filter = x > y ? x - y : 0;
  if (!action_matches_stub(&header, filter, &thrown, &offset, &is_catch,
                           &is_cleanup)) {
    failures++;
  }
  if (offset != 7 || !is_catch || is_cleanup) {
    failures++;
  }
}

static void test_stack_wide_arg(void) {
  accept_three_wide(1, 2, 4 > 1 ? 4 - 1 : 0);
}

static void test_call_return_merge(void) {
  long long a = 1;
  long long b = 0;
  accept_seven(a > b ? make_seven() : 0);
}

static int check_divmod(long long v) {
  long long seconds = v / 1000000;
  long long rem = v % 1000000;
  if (seconds < 0 || rem < 0 || rem >= 1000000) {
    return 1;
  }
  if ((v - rem) / 1000000 != seconds) {
    return 1;
  }
  return 0;
}

static void test_divmod_consecutive(void) {
  if (check_divmod(1700000000000000LL + 2000)) {
    failures++;
  }
  if (check_divmod(make_seven() + 1700000000000000LL + 1993)) {
    failures++;
  }
}

int main(void) {
  failures = 0;
  test_ternary_false_branch();
  test_ternary_true_branch();
  test_call_with_ternary_false();
  test_call_with_ternary_true();
  test_two_wide_merge_args();
  test_second_register_pair();
  test_pointer_interleave();
  test_libcall_producer();
  test_wide_load_producer();
  test_signextend_producer();
  test_true_signextend_producer();
  test_nested_ternary();
  test_action_matches_shape();
  test_stack_wide_arg();
  test_call_return_merge();
  test_divmod_consecutive();
  return failures;
}

typedef unsigned long long u64;

static u64 udivmoddi4(u64 n, u64 d, u64* rem) {
  u64 q = 0;
  u64 r = 0;
  for (int i = 63; i >= 0; i--) {
    r <<= 1;
    r |= (n >> i) & 1ULL;
    if (r >= d) {
      r -= d;
      q |= (1ULL << i);
    }
  }
  if (rem != 0) {
    *rem = r;
  }
  return q;
}

long long __divdi3(long long a, long long b) {
  int negate = 0;
  u64 ua, ub;
  if (a < 0) {
    ua = (u64)(-a);
    negate ^= 1;
  } else {
    ua = (u64)a;
  }
  if (b < 0) {
    ub = (u64)(-b);
    negate ^= 1;
  } else {
    ub = (u64)b;
  }
  u64 q = udivmoddi4(ua, ub, 0);
  return negate ? -(long long)q : (long long)q;
}

long long __moddi3(long long a, long long b) {
  int negate = 0;
  u64 ua, ub, r;
  if (a < 0) {
    ua = (u64)(-a);
    negate = 1;
  } else {
    ua = (u64)a;
  }
  if (b < 0) {
    ub = (u64)(-b);
  } else {
    ub = (u64)b;
  }
  (void)udivmoddi4(ua, ub, &r);
  return negate ? -(long long)r : (long long)r;
}

unsigned long long __muldi3(unsigned long long a, unsigned long long b) {
  unsigned long long result = 0;
  while (b != 0) {
    if (b & 1) {
      result += a;
    }
    a <<= 1;
    b >>= 1;
  }
  return result;
}
