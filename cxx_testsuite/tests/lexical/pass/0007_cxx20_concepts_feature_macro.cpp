// RUN: -std=c++20
#if __cpp_concepts != 202002L
#error "__cpp_concepts feature-test macro is missing"
#endif

int main(void) {
  return 0;
}
