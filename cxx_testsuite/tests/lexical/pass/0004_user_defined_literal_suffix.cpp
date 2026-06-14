// RUN: -std=c++11
int main(void) {
  return 123_km == 123 ? 0 : 1;
}
