// constexpr three-way comparison: the built-in scalar <=> is evaluated by the
// p-code constant-expression VM inside static_asserts, then the same logic runs
// at runtime so every backend's cmp3way lowering is exercised too.
#include <compare>

enum Color { red = 1, green = 2, blue = 3 };

constexpr int isign(int a, int b) {
  auto r = (a <=> b);
  if (r < 0) return -1;
  if (r > 0) return 1;
  return 0;
}

constexpr int usign(unsigned a, unsigned b) {
  auto r = (a <=> b);
  if (r < 0) return -1;
  if (r > 0) return 1;
  return 0;
}

constexpr int esign(Color a, Color b) {
  auto r = (a <=> b);
  if (r < 0) return -1;
  if (r > 0) return 1;
  return 0;
}

constexpr int dsign(double a, double b) {
  auto r = (a <=> b);
  if (r < 0) return -1;
  if (r > 0) return 1;
  return 0;
}

static_assert(isign(1, 2) == -1, "1 < 2");
static_assert(isign(2, 1) == 1, "2 > 1");
static_assert(isign(3, 3) == 0, "3 == 3");
static_assert(isign(-5, -3) == -1, "signed negatives");
static_assert(usign(1u, 0xFFFFFFFFu) == -1, "unsigned wraparound");
static_assert(usign(0xFFFFFFFFu, 1u) == 1, "unsigned wraparound rev");
static_assert(esign(red, blue) == -1, "enum less");
static_assert(esign(blue, red) == 1, "enum greater");
static_assert(dsign(1.0, 2.0) == -1, "double less");
static_assert(dsign(2.0, 1.0) == 1, "double greater");

int main() {
  if (isign(1, 2) != -1) return 1;
  if (isign(2, 1) != 1) return 2;
  if (isign(3, 3) != 0) return 3;
  if (isign(-5, -3) != -1) return 4;
  if (usign(1u, 0xFFFFFFFFu) != -1) return 5;
  if (usign(0xFFFFFFFFu, 1u) != 1) return 6;
  if (esign(red, blue) != -1) return 7;
  if (esign(blue, red) != 1) return 8;
  if (dsign(1.0, 2.0) != -1) return 9;
  if (dsign(2.0, 1.0) != 1) return 10;
  return 0;
}
