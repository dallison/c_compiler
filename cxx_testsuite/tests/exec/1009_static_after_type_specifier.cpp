// RUN: -std=c++17
// EXPECT_EXIT: 0

// Decl-specifiers may follow the type (`size_t static MakeTag(...)`).

struct Wire {
  int static tag(int field) { return field + 1; }
};

int main() { return Wire::tag(6) == 7 ? 0 : 1; }
