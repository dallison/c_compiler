// RUN: -std=c++2b

#if __cplusplus != 202302L
#error "c++2b does not select C++23 mode"
#endif

int main(void) {
  return 0;
}
