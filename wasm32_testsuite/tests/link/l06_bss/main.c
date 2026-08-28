// Uninitialized data shared between objects.  It has to be zero at startup
// and the two objects have to agree on where it is.

extern int scratch[64];
extern char flags[16];

void fill(void);

int main(void) {
  for (int i = 0; i < 64; i++) {
    if (scratch[i] != 0) {
      return 1;
    }
  }
  fill();
  int total = 0;
  for (int i = 0; i < 64; i++) {
    total += scratch[i];
  }
  return (total + flags[3]) & 0x7F;
}
