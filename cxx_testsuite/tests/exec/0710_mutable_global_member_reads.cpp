// RUN: -std=c++20
// EXPECT_EXIT: 0

// The members of a namespace-scope object that is not const are read at run
// time, even through a constexpr member function and even when the object
// was constant-initialized.  Only its address is a constant.

#include <string>

struct Flagged {
  char buf[4];
  const char* p;
  bool is_long;
  constexpr Flagged(bool l) : buf{}, p(nullptr), is_long(l) {}
  constexpr bool get_long() const { return is_long; }
  constexpr const char* first() const { return buf; }
};

struct Pointer {
  const char* d;
  int n;
  constexpr Pointer(const char* s) : d(s), n(1) {}
  Pointer(int v) : d(nullptr), n(v) {}
  constexpr const char* data() const { return d; }
  constexpr int count() const { return n; }
};

Flagged flagged[2] = {Flagged(false), Flagged(false)};
Pointer constant_init[2] = {Pointer("a"), Pointer("b")};
Pointer dynamic_init[2] = {Pointer(1), Pointer(2)};
std::string strings[] = {"a", std::string("bb"), "ccc"};

// Each check is a single expression, the form a constant fold would replace.
bool string_data_null() { return strings[1].data() == nullptr; }
unsigned long string_size() { return strings[2].size(); }
bool flagged_long() { return flagged[1].get_long(); }
bool flagged_first_null() { return flagged[1].first() == nullptr; }
bool constant_data_null() { return constant_init[1].data() == nullptr; }
int constant_count() { return constant_init[1].count(); }
bool dynamic_data_null() { return dynamic_init[1].data() == nullptr; }
bool dynamic_member_null() { return dynamic_init[1].d == nullptr; }
int dynamic_count() { return dynamic_init[0].count(); }

int main() {
  if (string_data_null() || string_size() != 3 || strings[1] != "bb") {
    return 1;
  }
  flagged[1].is_long = true;
  if (!flagged_long()) {
    return 2;
  }
  if (flagged_first_null()) {
    return 3;
  }
  constant_init[1].d = nullptr;
  constant_init[1].n = 5;
  if (!constant_data_null() || constant_count() != 5) {
    return 4;
  }
  dynamic_init[1].d = "x";
  if (dynamic_data_null() || dynamic_member_null()) {
    return 5;
  }
  if (dynamic_count() != 1) {
    return 6;
  }
  return 0;
}
