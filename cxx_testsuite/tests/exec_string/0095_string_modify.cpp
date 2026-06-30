// std::string modifiers: append, operator+=, push_back/pop_back, resize, clear,
// and the geometric growth that spills small strings onto the heap.
#include <string>

using std::string;

static bool eq(const char* a, const char* b) {
  while (*a != '\0' && *b != '\0') {
    if (*a != *b) return false;
    ++a;
    ++b;
  }
  return *a == *b;
}

int main() {
  string s("hello");
  s += ", world";
  if (!eq(s.c_str(), "hello, world")) return 1;
  s.push_back('!');
  if (!eq(s.c_str(), "hello, world!")) return 2;
  if (s.size() != 13) return 3;
  s.pop_back();
  if (!eq(s.c_str(), "hello, world")) return 4;

  string a("foo");
  a.append("bar");
  a.append(string("baz"));
  a.append(2, '!');
  if (!eq(a.c_str(), "foobarbaz!!")) return 5;

  // Grow well past the small-string buffer to force heap reallocation.
  string big;
  for (int i = 0; i < 100; ++i) {
    big += "ab";
  }
  if (big.size() != 200) return 6;
  if (big[0] != 'a' || big[199] != 'b') return 7;
  if (big.capacity() < big.size()) return 8;

  string r("trim me");
  r.resize(4);
  if (!eq(r.c_str(), "trim")) return 9;
  r.resize(6, 'x');
  if (!eq(r.c_str(), "trimxx")) return 10;

  r.clear();
  if (!r.empty()) return 11;
  if (r.size() != 0) return 12;

  string self("count");
  self += self;
  if (!eq(self.c_str(), "countcount")) return 13;

  string lhs("ab");
  string rhs("cd");
  lhs.swap(rhs);
  if (!eq(lhs.c_str(), "cd") || !eq(rhs.c_str(), "ab")) return 14;

  return 0;
}
