// RUN: -std=c++20

int main(void) {
  int value = 1;
  try {
    value = 3;
  } catch (...) {
    return 1;
  }
  return value == 3 ? 0 : 2;
}
