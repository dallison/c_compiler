// RUN: -std=c++20
// EXPECT_EXIT: 0

// A thrown class prvalue, whatever its form, becomes the exception object
// exactly once and is destroyed exactly once.
int live = 0, made = 0;
struct S { int v; S(int x) : v(x) { ++live; ++made; } S(const S& o) : v(o.v) { ++live; ++made; } S(S&& o) : v(o.v) { ++live; ++made; } ~S() { --live; } };
int t1(bool c) { try { throw c ? S(1) : S(2); } catch (const S& e) { return e.v; } return 0; }
int t2(const S& s) { try { throw static_cast<S>(s); } catch (const S& e) { return e.v; } return 0; }
int t3() { try { throw S(S(5)); } catch (const S& e) { return e.v; } return 0; }
int t4(bool c) { try { throw ((void)c, S(6)); } catch (const S& e) { return e.v; } return 0; }
int main() {
  if (t1(true) != 1 || live != 0) return 1;
  if (t1(false) != 2 || live != 0) return 2;
  { S s(3); if (t2(s) != 3 || live != 1) return 3; }
  if (live != 0) return 4;
  if (t3() != 5 || live != 0) return 5;
  if (t4(true) != 6 || live != 0) return 6;
  return 0;
}
