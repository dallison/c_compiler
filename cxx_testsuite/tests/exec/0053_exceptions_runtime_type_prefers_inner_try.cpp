// RUN: -std=c++20

int main(void) {
  try {
    try {
      throw 7;
      return 1;
    } catch (int value) {
      return value - 7;
    }
  } catch (...) {
    return 2;
  }
}
