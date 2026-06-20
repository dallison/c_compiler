// RUN: -std=c++20

int marker = 0;

int main(void) {
  int* thrown = &marker;
  try {
    throw thrown;
    return 1;
  } catch (int* value) {
    if (value == &marker) {
      return 0;
    }
    return 2;
  }
}
