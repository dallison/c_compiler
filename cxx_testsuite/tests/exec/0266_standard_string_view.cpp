// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <string_view>
#include <string>

using std::string_view;

int basics() {
  string_view sv = "hello world";
  if (sv.size() != 11 || sv.empty()) return 1;
  if (sv.front() != 'h' || sv.back() != 'd') return 2;
  if (sv[4] != 'o') return 3;
  if (sv.at(1) != 'e') return 4;

  string_view empty;
  if (!empty.empty() || empty.size() != 0) return 5;

  // data() points at the borrowed characters (no copy).
  const char* lit = "abc";
  string_view a(lit, 3);
  if (a.data() != lit || a.size() != 3) return 6;
  return 0;
}

int iteration() {
  string_view sv = "abc";
  int i = 0;
  for (char c : sv) {
    if (c != "abc"[i++]) return 10;
  }
  if (i != 3) return 11;

  // reverse
  string_view r = "abcd";
  const char* expect = "dcba";
  i = 0;
  for (auto it = r.rbegin(); it != r.rend(); ++it) {
    if (*it != expect[i++]) return 12;
  }
  if (i != 4) return 13;
  return 0;
}

int modifiers() {
  string_view sv = "hello world";
  sv.remove_prefix(6);
  if (sv != string_view("world")) return 20;
  sv = "hello world";
  sv.remove_suffix(6);
  if (sv != string_view("hello")) return 21;

  string_view a = "aaa";
  string_view b = "bbb";
  a.swap(b);
  if (a != string_view("bbb") || b != string_view("aaa")) return 22;
  return 0;
}

int operations() {
  string_view sv = "hello world";
  if (sv.substr(0, 5) != string_view("hello")) return 30;
  if (sv.substr(6) != string_view("world")) return 31;

  if (!sv.starts_with("hello")) return 32;
  if (sv.starts_with("world")) return 33;
  if (!sv.ends_with("world")) return 34;
  if (!sv.starts_with('h')) return 35;
  if (!sv.ends_with('d')) return 36;
  if (!sv.contains("lo w")) return 37;
  if (sv.contains("xyz")) return 38;

  if (sv.find("o") != 4) return 39;
  if (sv.find('o', 5) != 7) return 40;
  if (sv.find("zzz") != string_view::npos) return 41;
  if (sv.rfind('o') != 7) return 42;
  if (sv.find_first_of("aeiou") != 1) return 43;
  if (sv.find_last_of("aeiou") != 7) return 44;
  if (sv.find_first_not_of("hel") != 4) return 45;
  return 0;
}

int comparisons() {
  string_view a = "abc";
  string_view b = "abd";
  string_view c = "abc";
  if (!(a == c)) return 50;
  if (a == b) return 51;
  if (!(a < b)) return 52;
  if (b < a) return 53;
  if (!(a <= c) || !(a >= c)) return 54;
  if (a.compare(b) >= 0) return 55;
  return 0;
}

int string_interop() {
  std::string s = "greetings";
  // string -> string_view conversion.
  string_view sv = s;
  if (sv.size() != s.size() || sv != string_view("greetings")) return 60;
  if (sv.data() != s.data()) return 61;

  // string_view -> string construction.
  string_view part = sv.substr(0, 5);
  std::string built(part);
  if (built != "greet") return 62;
  return 0;
}

int main() {
  int rc;
  if ((rc = basics()) != 0) return rc;
  if ((rc = iteration()) != 0) return rc;
  if ((rc = modifiers()) != 0) return rc;
  if ((rc = operations()) != 0) return rc;
  if ((rc = comparisons()) != 0) return rc;
  if ((rc = string_interop()) != 0) return rc;
  return 0;
}
