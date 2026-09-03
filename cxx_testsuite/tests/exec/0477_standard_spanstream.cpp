// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <spanstream>

int main() {
  char storage[32] = {};
  std::ospanstream output{std::span<char>(storage, 32)};
  output << 42 << ' ' << 17;
  std::span<char> written = output.span();
  if (written.size() != 5 || storage[0] != '4' || storage[4] != '7') {
    return 1;
  }

  std::ispanstream input(written);
  int first = 0;
  int second = 0;
  input >> first >> second;
  if (first != 42 || second != 17) {
    return 2;
  }

  char combined_storage[16] = {};
  std::spanstream combined{std::span<char>(combined_storage, 16)};
  combined << 123;
  combined.seekg(0);
  int value = 0;
  combined >> value;
  if (value != 123 || combined.span().size() != 3) {
    return 3;
  }

  char short_storage[2] = {};
  std::ospanstream short_output{std::span<char>(short_storage, 2)};
  short_output << "abc";
  if (!short_output.fail() || short_output.span().size() != 2) {
    return 4;
  }
  return 0;
}
