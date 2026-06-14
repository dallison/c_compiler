// RUN: -std=c++14
int main(void) {
  return (1'000 == 1000) && (0b1010'0101 == 165) ? 0 : 1;
}
