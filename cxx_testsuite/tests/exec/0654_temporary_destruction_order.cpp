// RUN: -std=c++20
// EXPECT_EXIT: 0

// Full-expression temporaries are destroyed in the reverse order of their
// construction, including a temporary consumed by another's constructor, in
// a return statement, and when the full-expression throws.
#include <string.h>

static char log_buf[64];
static int log_len = 0;

struct T {
  char c;
  T(char c) : c(c) {}
  T(const T& o) : c(o.c) {}
  ~T() { log_buf[log_len++] = c; }
};
struct W {
  char c;
  W(const T& t) : c(t.c + 1) {}
  ~W() { log_buf[log_len++] = c; }
};

int use(const T&, const T&) { return 0; }
int use1(const W&) { return 0; }
int use2(const W&, const T&) { return 0; }
T make(char c) { return T(c); }
int f(const T& t) { return t.c; }
int g(const W& w) { return w.c; }
int boom(const W&, const T&) { throw 1; }

static bool expect(const char* want) {
  log_buf[log_len] = 0;
  bool ok = strcmp(log_buf, want) == 0;
  log_len = 0;
  return ok;
}

int ret_nested() { return g(W(T('a'))) + f(T('c')); }

int main() {
  use(T('a'), T('b'));
  if (!expect("ba")) return 1;
  use1(W(T('a')));
  if (!expect("ba")) return 2;
  use2(W(T('a')), T('c'));
  if (!expect("cba")) return 3;
  use(make('a'), T('b'));
  if (!expect("ba")) return 4;
  (void)(log_len >= 0 ? g(W(T('a'))) : 0);
  if (!expect("ba")) return 5;
  ret_nested();
  if (!expect("cba")) return 6;
  int x = f(T('a')) + use1(W(T('c')));
  (void)x;
  if (!expect("dca")) return 7;
  try {
    boom(W(T('a')), T('c'));
  } catch (int) {
  }
  if (!expect("cba")) return 8;
  return 0;
}
