int scratch[64];
char flags[16];

void fill(void) {
  for (int i = 0; i < 64; i++) {
    scratch[i] = i % 3;
  }
  flags[3] = 9;
}
