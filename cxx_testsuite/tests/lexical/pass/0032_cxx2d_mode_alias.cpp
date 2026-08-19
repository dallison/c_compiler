// RUN: -std=c++2d

#if __cplusplus != 202700L
#error "c++2d does not select C++29 mode"
#endif

int main(void) {
  return 0;
}
