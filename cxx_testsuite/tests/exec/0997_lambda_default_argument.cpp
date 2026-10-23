// RUN: -std=c++17
// EXPECT_EXIT: 0

int main() {
  auto value = [](int number = 3) { return number; };
  if (value() != 3) return 1;
  if (value(8) != 8) return 2;
  auto text = [&](const char* suffix = "") {
    return suffix[0] == 0 ? 1 : 2;
  };
  if (text() != 1) return 3;
  if (text("x") != 2) return 4;
  return 0;
}
