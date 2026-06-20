// RUN: -std=c++20

int main(void) {
  char thrown = 5;
  try {
    throw thrown;
    return 1;
  } catch (char value) {
    return value - 5;
  }
}
