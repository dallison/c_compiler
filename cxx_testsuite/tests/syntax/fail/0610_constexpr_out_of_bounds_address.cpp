// RUN: -std=c++20
// EXPECT: constexpr variable initializer is not a constant expression

// Only the outermost subscript may name the one-past-the-end element, and
// pointers into different rows are not elements of the same array.
int h[4];
int m[3][4];
struct S { int a[2]; };
S s;

constexpr int* past = &h[5];
constexpr int* before = &h[-1];
constexpr int* inner_past = &m[0][5];
constexpr int (*outer_past)[4] = &m[4];
constexpr int* row_past = &m[3][0];
constexpr int* member_past = &s.a[3];
constexpr long across_rows = &m[1][0] - &m[0][0];
constexpr bool past_row_is_next = &m[0][4] == &m[1][0];

int main() { return 0; }
