__thread int tls_zero;
__thread int tls_init = 1;
extern __thread int tls_extern;
__thread int tls_extern = 2;

int main(void) {
  return 0;
}
