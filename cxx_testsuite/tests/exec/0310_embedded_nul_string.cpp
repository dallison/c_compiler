// RUN: -std=c++23
// EXPECT_EXIT: 0

int main() {
  const char text[] = "a\0b";
  if (sizeof(text) != 4) {
    return 1;
  }
  if (text[0] != 'a' || text[1] != 0 || text[2] != 'b' || text[3] != 0) {
    return 2;
  }

  const char* concatenated = "x\0" "yz";
  if (concatenated[0] != 'x' || concatenated[1] != 0 ||
      concatenated[2] != 'y' || concatenated[3] != 'z' ||
      concatenated[4] != 0) {
    return 3;
  }
  return 0;
}
