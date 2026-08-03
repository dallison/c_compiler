// RUN: -std=c++14

#ifdef __cpp_deduction_guides
#error "__cpp_deduction_guides must not be defined before C++17"
#endif

int main(void) {
  return (1'000 == 1000) && (0b1010'0101 == 165) ? 0 : 1;
}
