// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <string>

int main() {
  std::string contents = "message\n}\n";
  contents.replace(contents.size() - 2, 2, ";\n");
  if (contents.size() != 10) {
    return 1;
  }
  if (contents[8] != ';') {
    return 2;
  }
  if (contents[9] != '\n') {
    return 3;
  }

  std::string counted = "abcdef";
  counted.replace(1, 3, "WXYZ", 2);
  if (counted != "aWXef") {
    return 4;
  }

  std::string replacement = "middle";
  counted.replace(1, 2, replacement);
  if (counted != "amiddleef") {
    return 5;
  }

  std::string overlapping = "abcdef";
  overlapping.replace(1, 2, overlapping.data() + 3, 3);
  return overlapping == "adefdef" ? 0 : 6;
}
