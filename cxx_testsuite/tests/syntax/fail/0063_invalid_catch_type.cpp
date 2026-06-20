// RUN: -std=c++20
// EXPECT: Invalid catch declaration type

void value(void) {
  try {
  } catch (void error) {
  }
}
