// RUN: -std=c++20

void helper(void) {
  throw 31;
}

int main(void) {
  try {
    helper();
    return 1;
  } catch (char value) {
    return 2;
  } catch (int value) {
    return value - 31;
  }
}
