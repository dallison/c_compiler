// Focused __thread local-exec runtime test for the x86-64 interpreter.
__thread int tls_init = 42;
__thread int tls_bss;

int main(void) {
  if (tls_init != 42) {
    return 1;
  }
  tls_init = 7;
  if (tls_init != 7) {
    return 2;
  }
  tls_bss = 99;
  if (tls_bss != 99) {
    return 3;
  }
  return 0;
}
