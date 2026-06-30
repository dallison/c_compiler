// std::string search (find/rfind), substr, compare, concatenation, and the
// comparison operators including operator<=>.
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
  string s("hello, world");

  if (s.find('o') != 4) return 1;
  if (s.find('o', 5) != 8) return 2;
  if (s.find('z') != string::npos) return 3;
  if (s.find("world") != 7) return 4;
  if (s.find(string("lo")) != 3) return 5;
  if (s.find("nope") != string::npos) return 6;
  if (s.rfind('o') != 8) return 7;
  if (s.rfind('l') != 10) return 8;

  string sub = s.substr(7, 5);
  if (!eq(sub.c_str(), "world")) return 9;
  string tail = s.substr(7);
  if (!eq(tail.c_str(), "world")) return 10;

  if (s.compare("hello, world") != 0) return 11;
  if (!(s.compare("hello") > 0)) return 12;
  if (!(string("abc").compare("abd") < 0)) return 13;

  // Non-member concatenation in every overload combination.
  string a("foo");
  string b("bar");
  if (!eq((a + b).c_str(), "foobar")) return 14;
  if (!eq((a + "X").c_str(), "fooX")) return 15;
  if (!eq(("Y" + a).c_str(), "Yfoo")) return 16;
  if (!eq((a + '!').c_str(), "foo!")) return 17;

  // Equality and ordering.
  if (!(a == "foo")) return 18;
  if (!(string("foo") == string("foo"))) return 19;
  if ((string("abc") <=> string("abd")) >= 0) return 20;
  if ((string("abd") <=> string("abc")) <= 0) return 21;
  if ((string("abc") <=> string("abc")) != 0) return 22;

  return 0;
}
