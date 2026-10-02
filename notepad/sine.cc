#include <cbbc>

/* Angle 0..255 is one cycle. Result is -64..64. */
static const unsigned char sine_q[65] = {
    0,  2,  3,  5,  6,  8,  9,  11, 12, 14, 16, 17, 19, 20, 22, 23,
    24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
    59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64, 64};

static int sine(unsigned char angle) {
  int neg = 0;
  int a = angle;
  int v;
  if (a >= 128) {
    neg = 1;
    a -= 128;
  }
  if (a > 64) {
    a = 128 - a;
  }
  v = sine_q[a];
  if (neg) {
    v = -v;
  }
  return v;
}

/* Scale -64..64 to about -400..400 graphics units, without a multiply. */
static int amplitude(int s) {
  int neg = 0;
  int dy;
  if (s < 0) {
    neg = 1;
    s = -s;
  }
  dy = (s << 2) + (s << 1) + (s >> 2);
  if (neg) {
    dy = -dy;
  }
  return dy;
}

int main() {
  int x;
  unsigned char angle = 0;
  unsigned char frac = 0;
  mode(4);
  gcol(BBC_GCOL_SET, 1);
  move(0, 512 + amplitude(sine(0)));
  for (x = 4; x < 1280; x += 4) {
    unsigned char next;
    /* 320 steps of 1 + 154/256 advance the phase by two cycles. */
    angle = angle + 1;
    next = frac + 154;
    if (next < frac) {
      angle = angle + 1;
    }
    frac = next;
    draw(x, 512 + amplitude(sine(angle)));
  }
  return 0;
}
