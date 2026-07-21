// RUN: -target 65c02

#include <cstdio>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> values;
  for (int i = 0; i < 10; ++i) {
    values.push_back("hello world " + std::to_string(i));
  }
  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i] != "hello world " + std::to_string(static_cast<int>(i))) {
      return 1;
    }
  }
  return 0;
}
