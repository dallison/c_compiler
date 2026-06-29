// Built-in scalar three-way comparison (operator<=>) across operand kinds.
// Exercises the cmp3way IR lowering for the active backend.
#include <compare>

using std::partial_ordering;
using std::strong_ordering;

enum Color { red = 1, green = 2, blue = 3 };

static int check_int() {
  if (!((1 <=> 2) < 0)) return 1;
  if (!((2 <=> 1) > 0)) return 2;
  if (!((5 <=> 5) == 0)) return 3;
  if (!((1 <=> 2) == strong_ordering::less)) return 4;
  if (!((2 <=> 1) == strong_ordering::greater)) return 5;
  if (!((5 <=> 5) == strong_ordering::equal)) return 6;
  // Negative signed values must use signed comparison.
  if (!((-1 <=> 0) < 0)) return 7;
  if (!((-5 <=> -3) < 0)) return 8;
  return 0;
}

static int check_unsigned() {
  unsigned big = 0xFFFFFFFFu;
  unsigned one = 1u;
  if (!((one <=> big) < 0)) return 10;   // 1 < 4294967295 (unsigned)
  if (!((big <=> one) > 0)) return 11;
  if (!((one <=> one) == 0)) return 12;
  return 0;
}

static int check_enum() {
  Color a = red;
  Color b = blue;
  if (!((a <=> b) < 0)) return 20;
  if (!((b <=> a) > 0)) return 21;
  if (!((a <=> a) == 0)) return 22;
  return 0;
}

static int check_pointer() {
  int arr[3];
  int* p0 = &arr[0];
  int* p2 = &arr[2];
  if (!((p0 <=> p2) < 0)) return 30;
  if (!((p2 <=> p0) > 0)) return 31;
  if (!((p0 <=> p0) == 0)) return 32;
  return 0;
}

static int check_double() {
  double zero = 0.0;
  double small = 1.0;
  double big = 2.0;
  if (!((small <=> big) < 0)) return 40;
  if (!((big <=> small) > 0)) return 41;
  if (!((small <=> small) == 0)) return 42;
  if (!((small <=> small) == partial_ordering::equivalent)) return 43;
  // NaN compares unordered with everything.
  double nan = zero / zero;
  partial_ordering r = (nan <=> small);
  if (!(r == partial_ordering::unordered)) return 44;
  if ((r < 0) || (r == 0) || (r > 0)) return 45;
  return 0;
}

static int check_float() {
  float small = 1.0f;
  float big = 2.0f;
  if (!((small <=> big) < 0)) return 50;
  if (!((big <=> small) > 0)) return 51;
  if (!((small <=> small) == 0)) return 52;
  return 0;
}

int main() {
  int rc = check_int();
  if (rc) return rc;
  rc = check_unsigned();
  if (rc) return rc;
  rc = check_enum();
  if (rc) return rc;
  rc = check_pointer();
  if (rc) return rc;
  rc = check_double();
  if (rc) return rc;
  rc = check_float();
  if (rc) return rc;
  return 0;
}
