#include <bbc.h>

/* MODE 4 keeps the screen at &5800. park_stack() moves the software
   stack to that address so later calls do not write into the picture. */
extern void park_stack(void);

static void plot8(int cx, int cy, int x, int y) {
  dot(cx + x, cy + y);
  dot(cx - x, cy + y);
  dot(cx + x, cy - y);
  dot(cx - x, cy - y);
  dot(cx + y, cy + x);
  dot(cx - y, cy + x);
  dot(cx + y, cy - x);
  dot(cx - y, cy - x);
}

/* Midpoint circle in BBC graphics units. The origin is the bottom left,
   and the visible range is 0..1279 by 0..1023. */
static void circle(int cx, int cy, int r) {
  int x = 0;
  int y = r;
  int d = 3 - (r << 1);

  while (x <= y) {
    plot8(cx, cy, x, y);
    if (d < 0) {
      d += (x << 2) + 6;
    } else {
      d += ((x - y) << 2) + 10;
      y--;
    }
    x++;
  }
}

int main(void) {
  mode(4);
  park_stack();
  gcol(BBC_GCOL_SET, 1);
  circle(640, 512, 400);
  for (;;) {
  }
  return 0;
}
