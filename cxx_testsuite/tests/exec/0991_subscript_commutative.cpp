// RUN: -std=c++17
// EXPECT_EXIT: 0

int main() {
  int values[4];
  values[0] = 7;
  values[1] = 8;
  values[2] = 9;
  values[3] = 10;
  if (0[values] != 7) return 1;
  if (3[values] != 10) return 2;
  unsigned count = sizeof(values) / sizeof(0[values]);
  if (count != 4) return 3;
  return 0;
}
