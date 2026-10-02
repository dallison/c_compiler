// Soft-float sine plot: two cycles across mode 4, amplitude 400 graphics
// units, using libc sin. Build with 6502 support/build_bbc_sine_fp.sh --plot.

#include <cbbc>
#include <math.h>

static const float k_phase_scale = 0.009817477f;  // (4*pi) / 1280

int main(void) {
  mode(4);
  gcol(BBC_GCOL_SET, 1);
  move(0, 512);
  for (int x = 8; x < 1280; x += 8) {
    float s = (float)sin((float)x * k_phase_scale);
    draw(x, 512 + (int)(s * 400.0f));
  }
  return 0;
}
