// RUN: -std=c++20

int main(void) {
  long long thrown = 123456789;
  try {
    throw thrown;
    return 1;
  } catch (long long value) {
    if (value == 123456789) {
      return 0;
    }
    return 2;
  }
}
