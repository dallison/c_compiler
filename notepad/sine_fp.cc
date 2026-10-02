// Soft-float sine table debug.
// - Hosted / 65c02 interpreter: printf (default).
// - BBC Micro: build with -DSINE_FP_BBC (see 6502 support/build_bbc_sine_fp.sh).
//   The printf build loads about 12KB: it fits the mode-7 layout (bbc.ld) but
//   not bbc_mode4.ld, whose image must end below HIMEM at &5800.

#include <math.h>
#include <stdio.h>

#if defined(SINE_FP_BBC)
#include <cbbc>

static void put_ch(char c) { oswrch((unsigned char)c); }

static void print_uint(unsigned int v) {
  char buf[10];
  int n = 0;
  if (v == 0) {
    put_ch('0');
    return;
  }
  while (v > 0) {
    buf[n++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (n > 0) {
    put_ch(buf[--n]);
  }
}

static void print_int(int v) {
  if (v < 0) {
    put_ch('-');
    v = -v;
  }
  print_uint((unsigned int)v);
}

static void print_row(int x, int y, int s_milli) {
  print_int(x);
  put_ch(' ');
  print_int(y);
  put_ch(' ');
  print_int(s_milli);
  osnewl();
}
#endif

static const float k_phase_scale = 0.009817477f;  // (4*pi) / 1280

int main(void) {
  for (unsigned char k = 0; k < 32; k = (unsigned char)(k + 1)) {
    int x = (int)k * 40;
    float s = (float)sin((float)x * k_phase_scale);
    int s_milli = (int)(s * 1000.0f);
    int bump = (s_milli + s_milli) / 5;
    int y = 512 + bump;
#if defined(SINE_FP_BBC)
    print_row(x, y, s_milli);
#else
    printf("%d %d %d\n", x, y, s_milli);
#endif
  }
  return 0;
}
