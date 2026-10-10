// Addresses of static array elements, up to one past the end, are constants.
// A row `m[i]` of a two-dimensional array is an array of its own.
int h[4];
int m[3][4];
struct S { int a[2]; };
S s;
constexpr int cm[2][3] = {{1, 2, 3}, {4, 5, 6}};

constexpr int* first = &h[0];
constexpr int* end = &h[4];
constexpr int* inner_end = &m[1][4];
constexpr int (*outer_end)[4] = &m[3];
constexpr int* member_end = &s.a[2];

static_assert(end - first == 4, "");
static_assert(end == h + 4, "");
static_assert(inner_end == &m[1][0] + 4, "");
static_assert(outer_end == m + 3, "");
static_assert(member_end - s.a == 2, "");

static_assert(&m[0][1] == &m[0][0] + 1, "");
static_assert(&m[1][3] == &m[1][0] + 3, "");
static_assert(&m[1][3] - &m[1][0] == 3, "");
static_assert(m[0] + 1 == &m[0][1], "");
static_assert(*(&m[1]) + 2 == &m[1][2], "");
static_assert(*(m + 2) + 1 - m[2] == 1, "");
static_assert(*m == m[0], "");
static_assert(m[0] != m[1], "");
static_assert(&m[2] - &m[0] == 2, "");

static_assert(*(cm[1] + 2) == 6, "");
static_assert((*(cm + 1))[0] == 4, "");
static_assert(&cm[1][2] - cm[1] == 2, "");
static_assert(**cm == 1, "");

int main() {
  if (end - first != 4) return 1;
  if (inner_end != &m[2][0]) return 2;
  if (outer_end - m != 3) return 3;
  if (member_end != s.a + 2) return 4;
  constexpr const int* p = cm[1] + 1;
  if (*p != 5) return 5;
  return 0;
}
