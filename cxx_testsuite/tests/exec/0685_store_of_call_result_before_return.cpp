// RUN: -std=c++20
// EXPECT_EXIT: 0
// `g = f(); return g;` stores the call's value after the call, so the call is
// not in tail position: as a tail call (a jump to f) the store was dropped.
static long last;
static long counter;

__attribute__((noinline)) long Next(long step) {
  counter += step;
  return counter;
}

__attribute__((noinline)) long RecordNext(long step) {
  last = Next(step);
  return last;
}

struct Pair { long a; long b; };
static Pair pair;

__attribute__((noinline)) long RecordMember(long step) {
  pair.b = Next(step);
  return pair.b;
}

int main() {
  if (RecordNext(5) != 5 || last != 5) return 1;
  if (RecordNext(2) != 7 || last != 7) return 2;
  if (RecordMember(3) != 10 || pair.b != 10) return 3;
  return 0;
}
