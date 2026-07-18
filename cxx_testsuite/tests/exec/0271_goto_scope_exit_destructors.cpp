// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// RAII across `goto`: jumping out of (or backward past) a scope must run the
// destructors of the automatic objects whose scope is exited, in reverse
// construction order, exactly like a fall-through block exit.  Objects still in
// scope at the target label must be left alone.

int g_ctors = 0;
int g_dtor_seq[64];
int g_dtor_n = 0;

struct T {
  int id;
  explicit T(int i) : id(i) { g_ctors++; }
  ~T() { g_dtor_seq[g_dtor_n++] = id; }
};

static void reset() {
  g_ctors = 0;
  g_dtor_n = 0;
}

// Forward goto out of a single nested block: the inner object is destroyed at
// the goto, the outer object survives until it falls out of its own block.
static int forward_inner() {
  reset();
  {
    T a(1);
    {
      T b(2);
      goto after;
    }
  after:
    (void)a.id;
  }  // `a` destroyed here on fall-through
  if (g_dtor_n != 2) return 1;
  if (g_dtor_seq[0] != 2) return 2;  // b at the goto
  if (g_dtor_seq[1] != 1) return 3;  // a at block end
  return 0;
}

// Forward goto out of several nested scopes at once (to a label outside them
// all): every exited object is destroyed at the goto, innermost first.
static int forward_multi() {
  reset();
  {
    T a(1);
    {
      T b(2);
      {
        T c(3);
        goto out;
      }
    }
  }
out:
  if (g_dtor_n != 3) return 10;
  if (g_dtor_seq[0] != 3) return 11;  // c
  if (g_dtor_seq[1] != 2) return 12;  // b
  if (g_dtor_seq[2] != 1) return 13;  // a
  return 0;
}

// Backward goto: objects declared after the label but before the goto go out of
// scope on every jump (and are reconstructed on re-entry); objects declared
// before the label stay alive.
static int backward() {
  reset();
  int iters = 0;
  {
    T a(1);
  loop:
    (void)iters;
    T b(2);
    ++iters;
    if (iters < 3) goto loop;  // two backward jumps, then fall through
  }
  // a: constructed once.  b: constructed on entry and on each re-entry (3x).
  if (g_ctors != 4) return 20;
  // b destroyed at the first goto, at the second goto, and at fall-through;
  // then a at the end of its block.
  if (g_dtor_n != 4) return 21;
  if (g_dtor_seq[0] != 2) return 22;
  if (g_dtor_seq[1] != 2) return 23;
  if (g_dtor_seq[2] != 2) return 24;
  if (g_dtor_seq[3] != 1) return 25;
  return 0;
}

// A goto that stays within a scope without leaving it destroys nothing extra.
static int forward_same_scope() {
  reset();
  {
    T a(1);
    goto skip;
    {
      T never(99);  // never constructed: goto jumps over it
      (void)never.id;
    }
  skip:
    (void)a.id;
  }
  if (g_ctors != 1) return 30;   // only a
  if (g_dtor_n != 1) return 31;  // only a, at block end
  if (g_dtor_seq[0] != 1) return 32;
  return 0;
}

int main() {
  int rc = forward_inner();
  if (rc) return rc;
  rc = forward_multi();
  if (rc) return rc;
  rc = backward();
  if (rc) return rc;
  rc = forward_same_scope();
  if (rc) return rc;
  return 0;
}
