// RUN: -std=c++23 -Werror=preprocessor

#if 0
#warning inactive warning must not be emitted
#endif

int main() {
  return 0;
}
