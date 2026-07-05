// RUN: -std=c++17
#ifdef __cpp_concepts
#error "__cpp_concepts should not be defined before C++20"
#endif

int main(void) {
  return 0;
}
