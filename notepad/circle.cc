#include <cbbc>

/* One horizontal chord of the disc. */
static void span(int x0, int x1, int y) {
  move(x0, y);
  draw(x1, y);
}

/* Filled midpoint circle. Each step of the outline gives the half-width
   of a row, and the eight-way symmetry covers every row from -r to r. */
static void circle(int cx, int cy, int r) {
  int x = 0;
  int y = r;
  int d = 3 - (r << 1);

  while (x <= y) {
    span(cx - x, cx + x, cy + y);
    span(cx - x, cx + x, cy - y);
    span(cx - y, cx + y, cy + x);
    span(cx - y, cx + y, cy - x);
    if (d < 0) {
      d += (x << 2) + 6;
    } else {
      d += ((x - y) << 2) + 10;
      y--;
    }
    x++;
  }
}

/* (&F2) points at the *RUN line, "PROG 2" ended by CR. The first word
   is the filename. "2" is green in the default MODE 2 palette. No
   number means white. */
static int colour_argument(void) {
  const unsigned char* text =
      *reinterpret_cast<const unsigned char* volatile*>(0x00F2);
  int n = 0;
  int saw_digit = 0;
  while (*text > ' ') {
    text++;
  }
  while (*text == ' ') {
    text++;
  }
  while (*text >= '0' && *text <= '9') {
    int digit = *text - '0';
    saw_digit = 1;
    n = n * 10 + digit;
    text++;
  }
  if (saw_digit == 0) {
    return 7;
  }
  if (n > 15) {
    n = 15;
  }
  return n;
}

int main() {
  mode(2);
  gcol(BBC_GCOL_SET, colour_argument());
  circle(640, 512, 400);
  for (;;) {
  }
  return 0;
}
