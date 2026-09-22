// RUN: -std=c++17
// EXPECT_EXIT: 0

#define X_VAL(id) static constexpr int id = 1;
X_VAL(c)
#undef X_VAL

#define X_VAL(e) return #e[0];
static char first(void) { X_VAL(c) }
#undef X_VAL

int main() { return first() == 'c' ? 0 : 1; }
