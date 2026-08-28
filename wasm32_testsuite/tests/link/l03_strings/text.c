const char* greeting = "hello";

int length(const char* s) {
  int n = 0;
  while (s[n] != '\0') {
    n++;
  }
  return n;
}

int first(void) {
  const char* other = "xyz";
  return other[0] - 'x' + 2;
}
