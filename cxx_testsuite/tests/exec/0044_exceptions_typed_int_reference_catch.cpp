// RUN: -std=c++20

int main(void) {
  try {
    throw 42;
    return 1;
  } catch (int& value) {
    value = value + 1;
    return value - 43;
  }
}
