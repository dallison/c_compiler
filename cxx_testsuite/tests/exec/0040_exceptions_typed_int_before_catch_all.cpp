// RUN: -std=c++20

int main(void) {
  try {
    throw 7;
    return 1;
  } catch (int value) {
    if (value == 7) {
      return 0;
    }
    return 2;
  } catch (...) {
    return 3;
  }
}
