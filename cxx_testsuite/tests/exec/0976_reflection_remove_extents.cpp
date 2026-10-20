// RUN: -std=c++26
// EXPECT_EXIT: 0
#include <meta>

using namespace std::meta;

static_assert(remove_all_extents(^^int[2][3]) == ^^int);
static_assert(remove_all_extents(^^const long[4][5][6]) == ^^const long);
static_assert(remove_all_extents(^^char[]) == ^^char);
static_assert(remove_all_extents(^^int) == ^^int);
static_assert(remove_extent(^^int[2][3]) == ^^int[3]);

int main() {
  typename [:remove_all_extents(^^double[7][8]):] d = 1.5;
  return d == 1.5 ? 0 : 1;
}
