// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <vector>

struct named_value {
  char first;

  explicit named_value(const char* text)
      : first(text == nullptr ? '\0' : text[0]) {}
};

int main() {
  std::vector<named_value> values;
  char name[] = "ok";
  values.emplace_back(name);
  if (values.size() != 1) {
    return 1;
  }
  values.emplace_back("yes");
  if (values.size() != 2) {
    return 2;
  }
  if (values[0].first != 'o') {
    return 3;
  }
  if (values[1].first != 'y') {
    return 4;
  }
  return 0;
}
