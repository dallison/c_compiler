// RUN: -std=c++20
// EXPECT_EXIT: 0

__attribute__((noinline))
char digit(unsigned value) {
  return static_cast<char>('0' + value);
}

int main() {
  char buffer[2] = {1, 2};
  char* output = buffer + 1;

  *--output = digit(0);

  if (output != buffer) return 1;
  if (buffer[0] != '0') return 2;
  if (buffer[1] != 2) return 3;
  return 0;
}
