// RUN: -std=c++20 -O0
// EXPECT_EXIT: 0

#include <map>
#include <string>
#include <vector>

int main() {
  constexpr int count = 12;
  std::vector<std::string> values;
  std::map<std::string, int> lookup;

  for (int i = 0; i < count; ++i) {
    values.push_back("hello world " + std::to_string(i));
    lookup["x" + std::to_string(i)] = i * 2;
  }

  if (values.size() != count || lookup.size() != count) {
    return 1;
  }
  for (int i = 0; i < count; ++i) {
    if (values[i] != "hello world " + std::to_string(i)) {
      return 2;
    }
    auto entry = lookup.find("x" + std::to_string(i));
    if (entry == lookup.end() || entry->second != i * 2) {
      return 3;
    }
  }
  return 0;
}
