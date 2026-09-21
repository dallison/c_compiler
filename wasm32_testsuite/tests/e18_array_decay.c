int first(const char* p) {
  if (p == 0) {
    return -1;
  }
  return (unsigned char)p[0];
}

int main(void) {
  volatile int pad = 1;
  unsigned long long fx[64];
  char text[512];
  fx[0] = 0;
  text[0] = '9';
  text[1] = 0;
  (void)pad;
  (void)fx[0];
  if (first(text) != '9') {
    return 1;
  }
  if (first(&text[0]) != '9') {
    return 2;
  }
  {
    const char* decayed = text;
    const char* indexed = &text[0];
    if (decayed == 0) {
      return 3;
    }
    if (indexed == 0) {
      return 4;
    }
    if (decayed != indexed) {
      return 5;
    }
    if (decayed[0] != '9') {
      return 6;
    }
  }
  return 0;
}
