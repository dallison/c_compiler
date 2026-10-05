// RUN: -std=c++20
// EXPECT_EXIT: 0

// A goto that leaves a catch handler ends that handler, as falling off its
// end does: the exception object is destroyed and is no longer current.
#include <exception>

static int alive = 0;
struct E {
  int v;
  E(int v) : v(v) { ++alive; }
  E(const E& o) : v(o.v) { ++alive; }
  ~E() { --alive; }
};

static bool no_current() { return std::current_exception() == nullptr; }

static int plain_goto() {
  try {
    throw E(1);
  } catch (E& e) {
    if (e.v != 1) return 1;
    goto out;
  }
  return 2;
out:
  if (alive != 0) return 3;
  if (!no_current()) return 4;
  return 0;
}

static int conditional_goto(int x) {
  try {
    throw E(2);
  } catch (E&) {
    if (x > 0) goto out;
    return 5;
  }
out:
  if (alive != 0) return 6;
  if (!no_current()) return 7;
  return 0;
}

static int nested_handlers() {
  try {
    throw E(3);
  } catch (E&) {
    try {
      throw E(4);
    } catch (E& inner) {
      if (inner.v != 4) return 8;
      if (alive != 2) return 9;
      goto out;
    }
  }
  return 10;
out:
  if (alive != 0) return 11;
  if (!no_current()) return 12;
  return 0;
}

static int goto_within_handler() {
  try {
    throw E(5);
  } catch (E& e) {
    goto inside;
  inside:
    if (alive != 1 || e.v != 5) return 13;
    if (no_current()) return 14;
  }
  if (alive != 0) return 15;
  return 0;
}

static int loop_retry() {
  int tries = 0;
again:
  try {
    ++tries;
    if (tries < 4) throw E(tries);
  } catch (E&) {
    goto again;
  }
  if (alive != 0) return 16;
  if (tries != 4) return 17;
  return 0;
}

int main() {
  if (int r = plain_goto()) return r;
  if (int r = conditional_goto(1)) return r;
  if (int r = nested_handlers()) return r;
  if (int r = goto_within_handler()) return r;
  if (int r = loop_retry()) return r;
  if (std::uncaught_exceptions() != 0) return 18;
  return 0;
}
