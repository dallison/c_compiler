// RUN: -std=c++20
// EXPECT_EXIT: 0

// When an exception leaves a full-expression, the temporaries already
// constructed in it are destroyed (and only those).  A thrown lvalue is
// copied into the exception object, so destroying the original does not
// affect the object the handler sees.
#include <stdexcept>
#include <string>

int live = 0;

struct S {
  std::string s;
  S(const char* p) : s(p) { ++live; }
  S(const S& o) : s(o.s) { ++live; }
  ~S() { --live; }
};

std::string g(const S& x) {
  if (x.s.size() > 2) throw std::runtime_error("a message too long for SSO");
  return x.s;
}
std::string f(const char* p) { return g(S(p)); }
std::string h(const char* p) {
  std::string local("local string that is quite long");
  return g(S(p)) + local;
}
int f2(const char* p) { return (int)g(S(p)).size(); }
int f3(bool c, const char* p) {
  return c ? (int)g(S(p)).size() : (int)S("xy").s.size();
}
void statement(const char* p) { g(S(p)) + g(S("ok")); }

struct Owner {
  int* count;
  Owner(int* c) : count(c) { ++*count; }
  Owner(const Owner& o) : count(o.count) { ++*count; }
  ~Owner() { --*count; }
};

int throw_lvalue() {
  int owners = 0;
  try {
    Owner o(&owners);
    throw o;
  } catch (const Owner& e) {
    if (owners != 1 || e.count != &owners) return 1;
  }
  return owners;
}

int main() {
  int r = 0;
  try { f("abc"); } catch (const std::exception&) { r |= 1; }
  try { h("abc"); } catch (const std::exception&) { r |= 2; }
  try { f2("abc"); } catch (const std::exception&) { r |= 4; }
  try { f3(true, "abc"); } catch (const std::exception&) { r |= 8; }
  try { statement("abc"); } catch (const std::exception&) { r |= 16; }
  if (r != 31) return 10;
  if (live != 0) return 11;
  if (f("ab") != "ab" || h("x") != "xlocal string that is quite long" ||
      f2("a") != 1 || f3(false, "abc") != 2 || f3(true, "a") != 1) {
    return 12;
  }
  if (live != 0) return 13;
  if (throw_lvalue() != 0) return 14;
  return 0;
}
