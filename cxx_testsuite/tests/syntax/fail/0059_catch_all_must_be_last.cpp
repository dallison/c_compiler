// RUN: -std=c++20
// EXPECT: catch (...) must be the last catch handler

int value(int input) {
  try {
    input += 1;
  } catch (...) {
    input = 0;
  } catch (int error) {
    input = error;
  }
  return input;
}
