// RUN: -std=c++17
// EXPECT_EXIT: 0

// Namespace-scope objects of one translation unit are initialized in
// declaration order whether they are default-constructed, constructed with
// arguments, or only need a destructor, and are destroyed in reverse order.
// The elements of an array are destroyed last to first.

#include <stdlib.h>

char events[64];
int count;
char next_default = 'c';

void Record(char event) { events[count++] = event; }

// Constructed first (trivially), so destroyed after every other object.
struct Last {
  ~Last() {
    Record('z');
    static const char kExpected[] = "abcdefgGFEDCBAz";
    for (int i = 0; kExpected[i] != '\0'; i++) {
      if (events[i] != kExpected[i]) {
        _Exit(10 + i);
      }
    }
    _Exit(count == 15 ? 0 : 2);
  }
} last;

struct V {
  char id;
  V() : id(next_default++) { Record(id); }
  V(char c) : id(c) { Record(id); }
  ~V() { Record(id - 'a' + 'A'); }
};

V first('a');
V array[] = {V('b'), V()};
V defaulted[2];
V converted[2] = {'f', 'g'};

int main() { return 3; }
