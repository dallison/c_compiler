// RUN: -std=c++20

int ThrowLeaf(void) {
  throw 42;
  return 1;
}

int Middle(void) {
  ThrowLeaf();
  return 2;
}

int main(void) {
  try {
    Middle();
    return 3;
  } catch (...) {
    return 0;
  }
}
