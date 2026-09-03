// RUN: -std=c++20
// EXPECT_EXIT: 0

int main() {
  int* values = new int[3]();
  if (values[0] != 0 || values[1] != 0 || values[2] != 0) return 1;
  values[1] = 7;
  if (values[1] != 7) return 2;
  delete[] values;
  return 0;
}
