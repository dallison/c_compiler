// RUN: -std=c++11
int main(void) {
  return (1 and 1) && (1 not_eq 0) ? 0 : 1;
}
