// RUN: -target 65c02 -std=c++20
// EXPECT: atomic operations are unavailable in the single-threaded 65(C)02 profile

#if __has_builtin(__atomic_load_n)
#error "65c02 must not advertise unavailable atomic builtins"
#endif

int main() {
  int value = 0;
  return __atomic_load_n(&value, 0);
}
