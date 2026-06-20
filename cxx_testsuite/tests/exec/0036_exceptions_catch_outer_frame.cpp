// RUN: -std=c++20

int ThrowFromHelper(void) {
  throw 42;
  return 1;
}

int main(void) {
  try {
    ThrowFromHelper();
    return 2;
  } catch (...) {
    return 0;
  }
}
