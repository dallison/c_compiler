// RUN: -std=c++20

int main(void) {
  try {
    throw 42;
    return 2;
  } catch (...) {
    return 0;
  }
  return 1;
}
