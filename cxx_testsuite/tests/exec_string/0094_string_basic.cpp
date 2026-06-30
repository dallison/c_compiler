// Core std::string construction, element access, capacity, and copy/move.
// Returns 0 on success; a nonzero code identifies the first failing check.
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
  string empty;
  if (!empty.empty()) return 1;
  if (empty.size() != 0) return 2;
  if (!eq(empty.c_str(), "")) return 3;

  string hello("hello");
  if (hello.size() != 5) return 4;
  if (!eq(hello.c_str(), "hello")) return 5;
  if (hello[0] != 'h' || hello[4] != 'o') return 6;
  if (hello.front() != 'h' || hello.back() != 'o') return 7;

  string from_n("abcdef", 3);
  if (!eq(from_n.c_str(), "abc")) return 8;

  string filled(4, 'z');
  if (!eq(filled.c_str(), "zzzz")) return 9;

  // Copy constructor must deep-copy: mutating the copy leaves the source intact.
  string copy = hello;
  copy[0] = 'H';
  if (!eq(hello.c_str(), "hello")) return 10;
  if (!eq(copy.c_str(), "Hello")) return 11;
  if (hello.data() == copy.data()) return 12;

  // Copy assignment is likewise independent.
  string assigned;
  assigned = hello;
  assigned[1] = 'E';
  if (!eq(hello.c_str(), "hello")) return 13;
  if (!eq(assigned.c_str(), "hEllo")) return 14;

  // Move construction transfers contents and empties the source.
  string movable("movable string that is long enough to be heap allocated");
  string moved = static_cast<string&&>(movable);
  if (!eq(moved.c_str(), "movable string that is long enough to be heap allocated"))
    return 15;
  if (!movable.empty()) return 16;

  return 0;
}
