// RUN: -std=c++20

int main(void) {
  int value = 7;
  int* ptr = &value;
  try {
    throw ptr;
    return 1;
  } catch (int*& caught) {
    *caught = 8;
    return value - 8;
  }
}
