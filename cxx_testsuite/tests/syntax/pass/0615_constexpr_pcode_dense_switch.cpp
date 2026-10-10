// RUN: -std=c++20 -fconstexpr-eval=pcode
// A dense switch is lowered to a computed branch into a table of branches,
// which the p-code evaluator runs like any other branch.
constexpr int from_zero(int w) {
  int v = 0;
  switch (w) {
    case 0: v = 10; break;
    case 1: v = 11; break;
    case 2: v = 12; break;
    case 3: v = 13; break;
    case 4: v = 14; break;
    case 5: v = 15; break;
    case 6: v = 16; break;
    case 7: v = 17; break;
  }
  return v;
}

constexpr int from_five(int w) {
  switch (w) {
    case 5: return 50;
    case 6: return 60;
    case 7: return 70;
    case 8: return 80;
    case 9: return 90;
    case 10: return 100;
    case 11: return 110;
    case 13: return 130;
    default: return -1;
  }
}

static_assert(from_zero(0) == 10);
static_assert(from_zero(1) == 11);
static_assert(from_zero(7) == 17);
static_assert(from_zero(8) == 0);
static_assert(from_five(4) == -1);
static_assert(from_five(5) == 50);
static_assert(from_five(11) == 110);
static_assert(from_five(12) == -1);
static_assert(from_five(13) == 130);
static_assert(from_five(14) == -1);

int main() { return 0; }
